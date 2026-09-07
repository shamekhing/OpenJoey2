#pragma once
// ── act/battle — the Battle Step and Damage Step (p.34-39) ──────────────────
#include "engine/action/State.hpp"
#include "engine/action/Moves.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/Catalog.hpp"

namespace openjoey::engine::action {

// Battle Step (p.35): declare an attack; target == nullptr -> direct attack.
// The attack is HELD OPEN (d.turnState.pending): ResolveDamage finishes it,
// ConfirmAttack re-validates (Replay rules, p.37).
inline std::string DeclareAttack(Duel &d, Card *c, Card *target) {
  d.traceBattle(protocol::BattleStep::AttackerChosen);
  if (!CanAttack(d, c))
    return "attack not possible (Battle Phase, your face-up ATK monster, once per Battle Phase).";
  if (d.turnState.pending.attacker)
    return "an attack is already held open — resolve or cancel it first.";
  // p.37 replay: the attack is refunded until re-declared. If the re-declared
  // attack uses a DIFFERENT monster, the original attacker has still declared
  // an attack this turn and cannot attack again.
  if (d.turnState.replayAttacker) {
    if (d.turnState.replayAttacker != c)
      d.turnState.attacked.insert(d.turnState.replayAttacker);
    d.turnState.replayAttacker = nullptr;
  }
  if (target) {
    zone::Zone_Monster *tz = d.field.monsterZoneOf(target);
    if (!tz || target->state.controller == c->state.controller)
      return "invalid attack target: an opponent's monster only.";
    d.turnState.pending = PendingAttack{c, target, false};
  } else {
    if (!OpponentFieldEmpty(d, c->state.controller))
      return "direct attack requires an empty opponent field (p.34).";
    d.turnState.pending = PendingAttack{c, nullptr, true};
  }
  d.traceBattle(target ? protocol::BattleStep::TargetChosen
                       : protocol::BattleStep::DirectDeclared);
  return c->name +
         (target ? " attacks " + target->name + "." : " attacks directly.");
}

// Cancel a held-open attack (player calls the attack off).
inline void CancelAttack(Duel &d) {
  d.turnState.pending = PendingAttack{};
  d.traceBattle(protocol::BattleStep::Cancelled);
}

// Replay check (p.37): is the held-open attack still valid? If false, the
// attack is cancelled — the monster has NOT used its attack yet.
inline bool ConfirmAttack(Duel &d) {
  d.traceBattle(protocol::BattleStep::ReplayCheck);
  auto &pending = d.turnState.pending;
  if (!pending.attacker) return false;
  zone::Zone_Monster *az = d.field.monsterZoneOf(pending.attacker);
  bool ok = az && pending.attacker->state.controller == d.turnPlayer;
  if (ok && pending.direct)
    ok = OpponentFieldEmpty(d, d.turnPlayer);
  else if (ok) {
    zone::Zone_Monster *tz = d.field.monsterZoneOf(pending.target);
    ok = tz && pending.target->state.controller != pending.attacker->state.controller;
  }
  if (!ok) {
    d.turnState.replayAttacker = pending.attacker; // remember for the p.37 lock
    d.turnState.pending = PendingAttack{};
    d.battleStep = protocol::BattleStep::Cancelled;
  }
  return ok;
}

// Damage Step (p.38), classic math on effective stats. Face-down defenders
// flip face-up first (visibility only); Flip effects enter the chain.
inline std::string ResolveDamage(Duel &d) {
  auto &pending = d.turnState.pending;
  if (!pending.attacker) return "no attack to resolve.";
  if (!ConfirmAttack(d))
    return "replay! attack cancelled — re-declare.";
  Card *a = pending.attacker;
  const int ap = a->state.controller;
  d.turnState.attacked.insert(a); // the attack is now committed
  std::string log = a->name;
  d.traceBattle(protocol::BattleStep::DamageBegin);
  d.damageStep = protocol::DamageStep::Calculate;

  if (pending.direct) {
    const int opp = 1 - ap;
    d.damageStep = protocol::DamageStep::Apply;
    d.lastDamageOutcome = protocol::DamageOutcome::DirectHit;
    Damage(d, opp, a->effectiveAtk());
    d.turnState.pending = PendingAttack{};
    d.damageStep = protocol::DamageStep::End;
    d.traceBattle(protocol::BattleStep::Resolved);
    CheckWinConditions(d);
    return log + " attacks directly for " + std::to_string(a->effectiveAtk()) + ".";
  }

  Card *t = pending.target;
  zone::Zone_Monster *tz = d.field.monsterZoneOf(t);
  d.damageStep = protocol::DamageStep::Flip;
  if (tz && !tz->isVisible()) {
    tz->changeVisibility(zone::Visibility::Visible);
    tz->changeOrientation(zone::Orientation::Horizontal);
    log += " flips " + t->name + " face-up;";
    for (const auto &e : classicEffectsFor(t->name)) {
      if (e.timing != EffectType::Trigger || e.id == ActionId::None) continue;
      if (const auto *ce = findClassicEffect(t->name); ce && ce->needsTarget)
        continue; // targeted flips resolve manually
      ActionArgs fa;
      if (e.id == ActionId::Move_ReturnHand)
        fa.target = a; // Wall of Illusion-style: return the attacker
      d.chain.push(e, t->state.controller, fa);
    }
  }
  const zone::Orientation defPos =
      tz ? tz->position() : zone::Orientation::Vertical;

  d.damageStep = protocol::DamageStep::Compare;
  if (defPos == zone::Orientation::Vertical) { // ATK vs ATK
    if (a->effectiveAtk() > t->effectiveAtk()) {
      d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKHigher;
      MoveDestroyToGY(d.field, t);
      Damage(d, t->state.controller, a->effectiveAtk() - t->effectiveAtk());
      log += " destroys " + t->name + " (" +
             std::to_string(a->effectiveAtk() - t->effectiveAtk()) + " damage).";
    } else if (a->effectiveAtk() < t->effectiveAtk()) {
      d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKLower;
      MoveDestroyToGY(d.field, a);
      Damage(d, ap, t->effectiveAtk() - a->effectiveAtk());
      log += " is destroyed by " + t->name + " (" +
             std::to_string(t->effectiveAtk() - a->effectiveAtk()) + " damage).";
    } else {
      d.lastDamageOutcome = protocol::DamageOutcome::AttackingMonsterATKEqual;
      MoveDestroyToGY(d.field, a);
      MoveDestroyToGY(d.field, t);
      log += " and " + t->name + " destroy each other.";
    }
  } else { // ATK vs DEF (p.38)
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
  d.turnState.pending = PendingAttack{};
  d.damageStep = protocol::DamageStep::End;
  d.traceBattle(protocol::BattleStep::Resolved);
  if (!d.config.chainResponseWindow)
    ResolveChain(d); // legacy: Flip effects (and responses) resolve now —
                     // p.45 mode leaves them to the response window
  CheckWinConditions(d);
  return log;
}

} // namespace openjoey::engine::action
