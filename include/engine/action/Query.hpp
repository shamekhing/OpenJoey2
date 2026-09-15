#pragma once
// ── act/Query — duel-state queries and guards (read-only; all take Duel&) ────
// Every legality predicate lives here. Guards NEVER mutate: a guard failure
// means the caller gets ActionResult::Fail before anything moved.
// Realization of an ActionId as a mat operation lives in Move.hpp / Action.hpp.
#include <string>

#include "action/ActionResult.hpp"
#include "engine/duel/Duel.hpp"

namespace openjoey::engine::action {

using cards::Card;
using openjoey::ActionResult;

// ── Life Points ──────────────────────────────────────────────────────────────
int Lp(const Duel &d, int player);
void Damage(Duel &d, int player, int n);
void GainLP(Duel &d, int player, int n);

// ── Win conditions (p.44); decided duels are never overwritten ───────────────
DuelResult CheckWinConditions(Duel &d);
void SetResult(Duel &d, DuelResult r, WinReason w);

// ── Legality predicates ──────────────────────────────────────────────────────
int TributesRequired(const Card *c);
bool CanNormalSummon(const Duel &d);
bool BattlePhaseOpen(const Duel &d);
bool OpponentFieldEmpty(const Duel &d, int player);

bool CanAttack(const Duel &d, Card *c);
bool CanDirectAttack(const Duel &d, Card *c);
bool AttackOpen(const Duel &d);
bool CanCancelAttack(const Duel &d);
bool HasNotAttacked(const Duel &d, Card *c);
bool CanFlipSummon(const Duel &d, Card *c);
bool CanChangePosition(const Duel &d, Card *c);
bool HasPlacedMonsterThisTurn(const Duel &d, int player);

// p.31: a Set Spell may be activated in a Main Phase even the turn it was
// Set; a Set Trap may not. The card must sit in its controller's S/T zone.
bool CanActivateSetSpellTrap(const Duel &d, const Card *c);

// ── Hand limit (End Phase, "until you have 6") ──────────────────────────────
int HandSize(const Duel &d, int player);
bool OverHandLimit(const Duel &d, int player);
bool CanEndTurn(const Duel &d, int player);
int DiscardToHandLimit(Duel &d, int player);
ActionResult ViewGraveyard(const Duel &d, int player);

}  // namespace openjoey::engine::action
