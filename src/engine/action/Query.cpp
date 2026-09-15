#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

int Lp(const Duel &d, int player) {
 return d.lp[player]; 
}

void Damage(Duel &d, int player, int n) {
    if (player < 0 || player >= 2 || n <= 0) return;
    d.lp[player] -= n;
}

void GainLP(Duel &d, int player, int n) {
    if (player < 0 || player >= 2 || n <= 0) return;
    d.lp[player] += n;
}

DuelResult CheckWinConditions(Duel &d) {
    if (d.result != DuelResult::Ongoing) return d.result;
    const bool p0 = d.lp[0] <= 0, p1 = d.lp[1] <= 0;
    if (p0 && p1) {
        d.result = DuelResult::Draw;
        d.winReason = WinReason::LPDepletion;
    } else if (p0) {
        d.result = DuelResult::Player1Win;
        d.winReason = WinReason::LPDepletion;
    } else if (p1) {
        d.result = DuelResult::Player0Win;
        d.winReason = WinReason::LPDepletion;
    }
    return d.result;
}

void SetResult(Duel &d, DuelResult r, WinReason w) {
    if (d.result == DuelResult::Ongoing) {
        d.result = r;
        d.winReason = w;
    }
}

int TributesRequired(const Card *c) {
    if (!c || !c->isMonster()) return 0;
    if (c->level >= 7) return 2;
    if (c->level >= 5) return 1;
    return 0;
}

bool CanNormalSummon(const Duel &d) {
    return d.result == DuelResult::Ongoing && !d.turnState.normalSummonUsed &&
           (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2);
}

bool BattlePhaseOpen(const Duel &d) {
    return d.result == DuelResult::Ongoing && d.turn.phase == Phase::Battle && !d.turn.skipBattle;
}

bool OpponentFieldEmpty(const Duel &d, int player) {
    const int opp = 1 - player;
    for (int z = 0; z < zone::Field::MONSTER_ZONES; ++z)
        if (!d.field.monsterZones[opp][z].isEmpty()) return false;
    return true;
}

bool CanAttack(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return BattlePhaseOpen(d) && mz && c->state.controller == d.turnPlayer &&
           mz->isVisible() && mz->orientation() == zone::Orientation::Vertical &&
           !d.turnState.attacked.count(c);
}

bool CanDirectAttack(const Duel &d, Card *c) {
 return CanAttack(d, c) && OpponentFieldEmpty(d, c->state.controller); 
}

bool AttackOpen(const Duel &d) {
 return d.turnState.pending.attacker != nullptr; 
}

bool CanCancelAttack(const Duel &d) {
 return AttackOpen(d); 
}

bool HasNotAttacked(const Duel &d, Card *c) {
 return !d.turnState.attacked.count(c); 
}

bool CanFlipSummon(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return d.result == DuelResult::Ongoing &&
           (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2) && mz &&
           c->state.controller == d.turnPlayer && !mz->isVisible() && !c->state.setThisTurn;
}

bool CanChangePosition(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return d.result == DuelResult::Ongoing &&
           (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2) && mz &&
           c->state.controller == d.turnPlayer && !c->state.placedThisTurn &&
           !c->state.setThisTurn && !d.turnState.flipSummoned.count(c) &&
           !d.turnState.attacked.count(c) && !d.turnState.positionChanged.count(c);
}

bool HasPlacedMonsterThisTurn(const Duel &d, int player) {
    for (const auto &mz : d.field.monsterZones[player])
        if (Card *c = mz.peek())
            if (c->state.placedThisTurn || c->state.setThisTurn) return true;
    return false;
}

bool CanActivateSetSpellTrap(const Duel &d, const Card *c) {
    if (!c || d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return false;
    auto [z, p] = d.field.findCard(c);
    if (!z || z->type() != zone::ZoneType::SpellTrap || c->state.controller != d.turnPlayer) return false;
    return !(c->isTrap() && c->state.setThisTurn);
}

int HandSize(const Duel &d, int player) {
 return d.field.handZones[player].count(); 
}

bool OverHandLimit(const Duel &d, int player) {
 return HandSize(d, player) > DuelConfig::HAND_LIMIT; 
}

bool CanEndTurn(const Duel &d, int player) {
    if (d.config.autoDiscardEndPhase) return true;  // EndTurn discards automatically down to 6
    return !OverHandLimit(d, player);               // player must discard first (p.41)
}

int DiscardToHandLimit(Duel &d, int player) {
    auto &hand = d.field.handZones[player];
    int n = 0;
    // Remove-then-put (the invariant moveTo upholds): the card must never sit
    // in two zones at once, even transiently.
    while (hand.count() > DuelConfig::HAND_LIMIT) {
        Card *c = hand.peek(-1);
        if (!c || !hand.remove(c)) break;
        d.field.graveyardZones[player].put(c);
        ++n;
    }
    return n;
}

ActionResult ViewGraveyard(const Duel &d, int player) {
    std::string out = "Graveyard (" + std::to_string(d.field.graveyardZones[player].count()) + "):";
    for (int i = 0; i < d.field.graveyardZones[player].count(); ++i)
        if (Card *c = d.field.graveyardZones[player].peek(i)) out += " " + c->name;
    return ActionResult::Ok(out + ".");
}

}  // namespace openjoey::engine::action
