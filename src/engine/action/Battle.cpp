#include "engine/action/Battle.hpp"

namespace openjoey::engine::action {

ActionResult DeclareAttack(Duel &d, Card *c, Card *target) {
    d.traceBattle(protocol::BattleStep::AttackerChosen);
    if (!CanAttack(d, c))
        return ActionResult::Fail(
            "attack not possible (Battle Phase, your face-up ATK monster, "
            "once per Battle Phase).");
    if (d.turnState.pending.attacker) return ActionResult::Fail("an attack is already held open — resolve or cancel it first.");
    // p.37 replay: the attack is refunded until re-declared. If the re-declared
    // attack uses a DIFFERENT monster, the original attacker has still declared
    // an attack this turn and cannot attack again.
    if (d.turnState.replayAttacker) {
        if (d.turnState.replayAttacker != c) d.turnState.attacked.insert(d.turnState.replayAttacker);
        d.turnState.replayAttacker = nullptr;
    }
    if (target) {
        zone::Zone *tz = d.field.monsterZoneOf(target);
        if (!tz || target->state.controller == c->state.controller) return ActionResult::Fail("invalid attack target: an opponent's monster only.");
        d.turnState.pending = PendingAttack{c, target, false};
    } else {
        if (!OpponentFieldEmpty(d, c->state.controller)) return ActionResult::Fail("direct attack requires an empty opponent field (p.34).");
        d.turnState.pending = PendingAttack{c, nullptr, true};
    }
    d.traceBattle(target ? protocol::BattleStep::TargetChosen : protocol::BattleStep::DirectDeclared);
    return ActionResult::Ok(c->name + (target ? " attacks " + target->name + "." : " attacks directly."));
}

void CancelAttack(Duel &d) {
    d.turnState.pending = PendingAttack{};
    d.traceBattle(protocol::BattleStep::Cancelled);
}

bool ConfirmAttack(Duel &d) {
    d.traceBattle(protocol::BattleStep::ReplayCheck);
    auto &pending = d.turnState.pending;
    if (!pending.attacker) return false;
    zone::Zone *az = d.field.monsterZoneOf(pending.attacker);
    bool ok = az && pending.attacker->state.controller == d.turnPlayer;
    if (ok && pending.direct) ok = OpponentFieldEmpty(d, d.turnPlayer);
    else if (ok) {
        zone::Zone *tz = d.field.monsterZoneOf(pending.target);
        ok = tz && pending.target->state.controller != pending.attacker->state.controller;
    }
    if (!ok) {
        d.turnState.replayAttacker = pending.attacker;  // remember for the p.37 lock
        d.turnState.pending = PendingAttack{};
        d.battleStep = protocol::BattleStep::Cancelled;
    }
    return ok;
}

ActionResult ResolveDamage(Duel &d) {
    auto &pending = d.turnState.pending;
    if (!pending.attacker) return ActionResult::Fail("no attack to resolve.");
    if (!ConfirmAttack(d)) return ActionResult::Fail("replay check failed — attack cancelled.");
    d.traceBattle(protocol::BattleStep::DamageBegin);
    d.damageStep = protocol::DamageStep::Calculate;
    Card *a = pending.attacker;
    const int ap = a->state.controller;
    std::string log = a->name;

    if (pending.direct) {
        d.lastDamageOutcome = protocol::DamageOutcome::DirectHit;
        Damage(d, 1 - ap, a->effectiveAtk());
        d.turnState.attacked.insert(a);
        d.turnState.pending = PendingAttack{};
        d.damageStep = protocol::DamageStep::End;
        d.traceBattle(protocol::BattleStep::Resolved);
        CheckWinConditions(d);
        return ActionResult::Ok(log + " attacks directly for " + std::to_string(a->effectiveAtk()) + ".");
    }

    Card *t = pending.target;
    zone::Zone *tz = d.field.monsterZoneOf(t);
    d.damageStep = protocol::DamageStep::Flip;
    if (tz && !tz->isVisible()) {
        tz->changeVisibility(zone::Visibility::Visible);
        tz->changeOrientation(zone::Orientation::Horizontal);
        log += " flips " + t->name + " face-up;";
        d.pendingTriggers.push_back(t);  // flip effects: spec-provider's concern
    }
    const zone::Orientation defPos = tz ? tz->orientation() : zone::Orientation::Vertical;

    d.damageStep = protocol::DamageStep::Compare;
    if (defPos == zone::Orientation::Vertical) {  // ATK vs ATK
        if (a->effectiveAtk() > t->effectiveAtk()) {
            d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKHigher;
            MoveDestroyToGY(d.field, t);
            Damage(d, t->state.controller, a->effectiveAtk() - t->effectiveAtk());
            log += " destroys " + t->name + " (" + std::to_string(a->effectiveAtk() - t->effectiveAtk()) + " damage).";
        } else if (a->effectiveAtk() < t->effectiveAtk()) {
            d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKLower;
            MoveDestroyToGY(d.field, a);
            Damage(d, ap, t->effectiveAtk() - a->effectiveAtk());
            log += " is destroyed by " + t->name + " (" + std::to_string(t->effectiveAtk() - a->effectiveAtk()) + " damage).";
        } else {
            d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKEqual;
            MoveDestroyToGY(d.field, a);
            MoveDestroyToGY(d.field, t);
            log += " and " + t->name + " destroy each other.";
        }
    } else {  // ATK vs DEF (p.38)
        if (a->effectiveAtk() > t->effectiveDef()) {
            d.lastDamageOutcome = protocol::DamageOutcome::DefenseLower;
            MoveDestroyToGY(d.field, t);
            log += " destroys the defending " + t->name + " (no damage).";
        } else if (a->effectiveAtk() < t->effectiveDef()) {
            d.lastDamageOutcome = protocol::DamageOutcome::DefenseHigher;
            Damage(d, ap, t->effectiveDef() - a->effectiveAtk());
            log += " rebounds " + std::to_string(t->effectiveDef() - a->effectiveAtk()) + " damage.";
        } else {
            d.lastDamageOutcome = protocol::DamageOutcome::DefenseEqual;
            log += " vs " + t->name + " (DEF): tied — nothing happens.";
        }
    }
    d.damageStep = protocol::DamageStep::Apply;
    d.turnState.attacked.insert(a);
    d.turnState.pending = PendingAttack{};
    d.damageStep = protocol::DamageStep::End;
    d.traceBattle(protocol::BattleStep::Resolved);
    if (!d.config.chainResponseWindow)
        ResolveChain(d);  // legacy: flip effects (and responses) resolve now —
                          // p.45 mode leaves them to the response window
    CheckWinConditions(d);
    return ActionResult::Ok(log);
}

}  // namespace openjoey::engine::action
