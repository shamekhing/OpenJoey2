#pragma once
// ── act/turn — the turn protocol: start, draw, phases, end ──────────────────
#include "engine/action/State.hpp"
#include "engine/action/Moves.hpp"

namespace openjoey::engine::action {

inline void ResetPerTurnState(Duel &d) {
  d.turnState = TurnState{};
  for (int p = 0; p < zone::Field::PLAYERS; ++p) {
    for (auto &mz : d.field.monsterZones[p])
      if (Card *c = mz.peek())
        c->state.setThisTurn = c->state.placedThisTurn = false;
    for (auto &st : d.field.spellTrapZones[p])
      if (Card *c = st.peek())
        c->state.setThisTurn = c->state.placedThisTurn = false;
  }
}

inline void ToMain1(Duel &d) { d.turn.phase = Phase::Main1; }
inline void ToBattle(Duel &d) { d.turn.phase = Phase::Battle; }
inline void ToMain2(Duel &d) { d.turn.phase = Phase::Main2; }

inline std::string ToMain1S(Duel &d) {
  if (d.result != DuelResult::Ongoing) return "the duel is over.";
  if (d.turn.phase == Phase::Main1) return "already in Main Phase 1.";
  if (d.turn.phase != Phase::Draw) return "cannot return to Main Phase 1.";
  ToMain1(d);
  return "Main Phase 1.";
}
inline std::string ToBattleS(Duel &d) {
  if (d.result != DuelResult::Ongoing) return "the duel is over.";
  if (d.turn.phase != Phase::Main1)
    return "Battle Phase is entered from Main Phase 1.";
  if (d.turn.skipBattle) return "no Battle Phase on the opening turn.";
  ToBattle(d);
  return "Battle Phase.";
}
inline std::string ToMain2S(Duel &d) {
  if (d.result != DuelResult::Ongoing) return "the duel is over.";
  if (d.turn.phase != Phase::Battle)
    return "Main Phase 2 follows the Battle Phase.";
  ToMain2(d);
  return "Main Phase 2.";
}

inline std::string DrawForTurn(Duel &d) {
  int p = d.turnPlayer;
  if (d.field.deckZones[p].isEmpty()) {
    d.result = (p == 0) ? DuelResult::Player1Win : DuelResult::Player0Win;
    d.winReason = WinReason::DeckOut;
    return "deck out — player " + std::to_string(p) + " loses.";
  }
  int n = MoveDraw(d.field, p, 1);
  return "player " + std::to_string(p) + " draws " + std::to_string(n) + ".";
}

inline std::string StartTurn(Duel &d) {
  ResetPerTurnState(d);
  d.turn.phase = Phase::Draw;
  if (d.turn.turnNumber == 1) {
    d.turn.skipDraw = true;
    d.turn.skipBattle = true;
    return "turn 1: draw skipped (starting player).";
  }
  if (d.turn.skipDraw) return "draw skipped.";
  return DrawForTurn(d);
}

inline std::string EndTurn(Duel &d) {
  int p = d.turnPlayer;
  if (!d.config.autoDiscardEndPhase && OverHandLimit(d, p))
    return "cannot end turn — discard down to 6 cards first (p.41).";
  int discarded = d.config.autoDiscardEndPhase ? DiscardToHandLimit(d, p) : 0;
  std::string msg = OverHandLimit(d, p)
                        ? "cannot end turn — hand limit unresolved."
                        : "hand limit OK";
  if (discarded) msg += " (" + std::to_string(discarded) + " discarded)";
  msg += ".";
  d.turnPlayer = 1 - d.turnPlayer;
  d.turn.phase = Phase::Draw;
  ++d.turn.turnNumber;
  d.turn.skipDraw = false;
  d.turn.skipBattle = false;
  ResetPerTurnState(d);
  return msg;
}

inline void SetDeck(Duel &d, int player, const std::vector<Card *> &cards) {
  for (Card *c : cards) {
    c->state.owner = c->state.controller = player;
    d.field.deckZones[player].put(c);
  }
}
inline void ShuffleDecks(Duel &d) {
  d.field.deckZones[0].shuffle();
  d.field.deckZones[1].shuffle();
}
inline void DrawOpeningHands(Duel &d, int n = DuelConfig::START_HAND) {
  for (int p = 0; p < zone::Field::PLAYERS; ++p)
    MoveDraw(d.field, p, n);
}

} // namespace openjoey::engine::action
