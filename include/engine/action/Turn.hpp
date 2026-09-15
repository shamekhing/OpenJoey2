#pragma once
// ── act/turn — the turn protocol: start, draw, phases, end, standby ─────────
#include <algorithm>

#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

void ResetPerTurnState(Duel &d);

void ToMain1(Duel &d);
void ToBattle(Duel &d);
void ToMain2(Duel &d);

ActionResult ToMain1S(Duel &d);
ActionResult ToBattleS(Duel &d);
ActionResult ToMain2S(Duel &d);

ActionResult DrawForTurn(Duel &d);

ActionResult StartTurn(Duel &d);

ActionResult EndTurn(Duel &d);

// Standby scan: face-up monsters the turn player controls are surfaced in
// d.pendingTriggers — whether they have an effect is a spec-provider concern
// (no hardcoded card knowledge in the engine).
ActionResult ResolveStandby(Duel &d);

void SetDeck(Duel &d, int player, const std::vector<Card *> &cards);
void ShuffleDecks(Duel &d);
void DrawOpeningHands(Duel &d, int n = DuelConfig::START_HAND);

// ── equip verbs (guarded composites over Move.hpp equip primitives) ─────────
ActionResult EquipCard(Duel &d, Card *equip, Card *monster);
ActionResult UnequipCard(Duel &d, Card *equip);

}  // namespace openjoey::engine::action
