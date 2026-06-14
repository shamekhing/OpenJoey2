#pragma once
#include "Type.hpp"

namespace openjoey::game {

/**
 * Tracks current phase, turn player, and first-turn battle skipping.
 */
class PhaseManager {
public:
  etypes::phase phase = etypes::phase::Draw;
  int     turnPlayer   = 0;   // 0 or 1
  int     turnNumber   = 1;
  bool    isFirstTurn  = true; // player 0's very first turn

  // Step to the next phase. Returns the phase we just entered.
  etypes::phase advance() {
    switch (phase) {
    case etypes::phase::Draw:    phase = etypes::phase::Standby; break;
    case etypes::phase::Standby: phase = etypes::phase::Main1;   break;
    case etypes::phase::Main1:
      // First turn player 0 skips Battle Phase
      phase = (isFirstTurn && turnPlayer == 0) ? etypes::phase::Main2
                                               : etypes::phase::Battle;
      break;
    case etypes::phase::Battle:  phase = etypes::phase::Main2;   break;
    case etypes::phase::Main2:   phase = etypes::phase::End;     break;
    case etypes::phase::End:
      endTurn();
      phase = etypes::phase::Draw;
      break;
    }
    return phase;
  }

  // Jump straight to End Phase (e.g. when a player surrenders or a card
  // effect forces the turn to end).
  void forceEnd() { phase = etypes::phase::End; }

  bool isMain() const {
    return phase == etypes::phase::Main1 || phase == etypes::phase::Main2;
  }
  bool isBattle() const { return phase == etypes::phase::Battle; }

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
