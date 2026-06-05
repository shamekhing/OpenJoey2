#pragma once
#include "Type.hpp"

namespace openjoey::game {

// ─── PhaseManager ─────────────────────────────────────────────────────────────
// Tracks the current phase and turn within a duel.
// advance() steps through the phase sequence for the active player.
// After End Phase the turn flips and the sequence restarts at Draw.

class PhaseManager {
public:
  Phase   phase        = Phase::Draw;
  int     turnPlayer   = 0;   // 0 or 1
  int     turnNumber   = 1;
  bool    isFirstTurn  = true; // player 0's very first turn

  // Step to the next phase. Returns the phase we just entered.
  Phase advance() {
    switch (phase) {
    case Phase::Draw:    phase = Phase::Standby; break;
    case Phase::Standby: phase = Phase::Main1;   break;
    case Phase::Main1:
      // First turn player 0 skips Battle Phase
      phase = (isFirstTurn && turnPlayer == 0) ? Phase::Main2 : Phase::Battle;
      break;
    case Phase::Battle:  phase = Phase::Main2;   break;
    case Phase::Main2:   phase = Phase::End;      break;
    case Phase::End:
      endTurn();
      phase = Phase::Draw;
      break;
    }
    return phase;
  }

  // Jump straight to End Phase (e.g. when a player surrenders or a card
  // effect forces the turn to end).
  void forceEnd() { phase = Phase::End; }

  bool isMain()   const { return phase == Phase::Main1 || phase == Phase::Main2; }
  bool isBattle() const { return phase == Phase::Battle; }

private:
  void endTurn() {
    turnPlayer  = 1 - turnPlayer;
    turnNumber += 1;
    if (isFirstTurn && turnPlayer == 0)
      isFirstTurn = false; // both players have had their first turn
    // first-turn flag stays true until player 0 takes their second turn
    if (turnNumber > 2)
      isFirstTurn = false;
  }
};

} // namespace openjoey::game
