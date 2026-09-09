#pragma once
// ── act/state — duel-state queries and ops (all take Duel& explicitly) ──────
#include <string>

#include "action/ActionResult.hpp"
#include "engine/duel/Duel.hpp"

namespace openjoey::engine::action {

using cards::Card;
using openjoey::ActionResult;

// ── Life Points ─────────────────────────────────────────────────────────────
inline int Lp(const Duel &d, int player) { return d.lp[player]; }
inline void Damage(Duel &d, int player, int n) {
    if (player < 0 || player >= 2 || n <= 0) return;
    d.lp[player] -= n;
}
inline void GainLP(Duel &d, int player, int n) {
    if (player < 0 || player >= 2 || n <= 0) return;
    d.lp[player] += n;
}

// ── Win conditions (p.44); decided duels are never overwritten ──────────────
inline DuelResult CheckWinConditions(Duel &d) {
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
inline void SetResult(Duel &d, DuelResult r, WinReason w) {
    if (d.result == DuelResult::Ongoing) {
        d.result = r;
        d.winReason = w;
    }
}
inline ActionResult ClassicGate(const char *mechanic) { return ActionResult::Fail(std::string(mechanic) + " are not legal in the classic format."); }

// ── Legality predicates ─────────────────────────────────────────────────────
inline int TributesRequired(const Card *c) {
    if (!c || !c->isMonster()) return 0;
    if (c->level >= 7) return 2;
    if (c->level >= 5) return 1;
    return 0;
}
inline bool CanNormalSummon(const Duel &d) { return d.result == DuelResult::Ongoing && !d.turnState.normalSummonUsed && (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2); }
inline bool BattlePhaseOpen(const Duel &d) { return d.result == DuelResult::Ongoing && d.turn.phase == Phase::Battle && !d.turn.skipBattle; }
inline bool OpponentFieldEmpty(const Duel &d, int player) {
    const int opp = 1 - player;
    for (int z = 0; z < zone::Field::MONSTER_ZONES; ++z)
        if (!d.field.monsterZones[opp][z].isEmpty()) return false;
    return true;
}

inline bool CanAttack(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return BattlePhaseOpen(d) && mz && c->state.controller == d.turnPlayer && mz->isVisible() && mz->position() == zone::Orientation::Vertical && !d.turnState.attacked.count(c);
}
inline bool CanDirectAttack(const Duel &d, Card *c) { return CanAttack(d, c) && OpponentFieldEmpty(d, c->state.controller); }
inline bool AttackOpen(const Duel &d) { return d.turnState.pending.attacker != nullptr; }
inline bool CanCancelAttack(const Duel &d) { return AttackOpen(d); }
inline bool HasNotAttacked(const Duel &d, Card *c) { return !d.turnState.attacked.count(c); }
inline bool CanFlipSummon(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return d.result == DuelResult::Ongoing && (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2) && mz && c->state.controller == d.turnPlayer && !mz->isVisible() && !c->state.setThisTurn;
}
inline bool CanChangePosition(const Duel &d, Card *c) {
    auto *mz = d.field.monsterZoneOf(c);
    return d.result == DuelResult::Ongoing && (d.turn.phase == Phase::Main1 || d.turn.phase == Phase::Main2) && mz && c->state.controller == d.turnPlayer && mz->isVisible() && !c->state.placedThisTurn && !c->state.setThisTurn && !d.turnState.flipSummoned.count(c) && !d.turnState.attacked.count(c) && !d.turnState.positionChanged.count(c);
}
inline bool HasPlacedMonsterThisTurn(const Duel &d, int player) {
    for (const auto &mz : d.field.monsterZones[player])
        if (Card *c = mz.peek())
            if (c->state.placedThisTurn || c->state.setThisTurn) return true;
    return false;
}

// p.31: a Set Spell may be activated in a Main Phase even the turn it was
// Set; a Set Trap may not. The card must sit in its controller's S/T zone.
inline bool CanActivateSetSpellTrap(const Duel &d, const Card *c) {
    if (!c || d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return false;
    auto [z, p] = d.field.findCard(c);
    if (!z || z->type() != zone::ZoneType::SpellTrap || c->state.controller != d.turnPlayer) return false;
    return !(c->isTrap() && c->state.setThisTurn);
}

// ── Hand limit (End Phase, "until you have 6") ──────────────────────────────
inline int HandSize(const Duel &d, int player) { return d.field.handZones[player].count(); }
inline bool OverHandLimit(const Duel &d, int player) { return HandSize(d, player) > DuelConfig::HAND_LIMIT; }
inline bool CanEndTurn(const Duel &d, int player) {
    if (d.config.autoDiscardEndPhase) return true;  // EndTurn discards automatically down to 6
    return !OverHandLimit(d, player);               // player must discard first (p.41)
}
inline int DiscardToHandLimit(Duel &d, int player) {
    auto &hand = d.field.handZones[player];
    int n = 0;
    // Remove-then-put (the invariant moveCard/moveTo uphold): the card must
    // never sit in two zones at once, even transiently.
    while (hand.count() > DuelConfig::HAND_LIMIT) {
        Card *c = hand.peek(-1);
        if (!c || !hand.remove(c)) break;
        d.field.graveyardZones[player].put(c);
        ++n;
    }
    return n;
}
inline ActionResult ViewGraveyard(const Duel &d, int player) {
    std::string out = "Graveyard (" + std::to_string(d.field.graveyardZones[player].count()) + "):";
    for (int i = 0; i < d.field.graveyardZones[player].count(); ++i)
        if (Card *c = d.field.graveyardZones[player].peek(i)) out += " " + c->name;
    return ActionResult::Ok(out + ".");
}

}  // namespace openjoey::engine::action
