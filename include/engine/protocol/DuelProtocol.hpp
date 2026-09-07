#pragma once
#include <cstdint>
#include <string>

namespace openjoey::engine {

// ── Turn Structure (Rulebook p.30) ──────────────────────────────────────────
// Draw -> Standby -> Main1 -> Battle -> Main2 -> End  (repeats)
enum class Phase : uint8_t { Draw, Standby, Main1, Battle, Main2, End };

inline const char *phaseName(Phase p) {
  switch (p) {
  case Phase::Draw: return "Draw";
  case Phase::Standby: return "Standby";
  case Phase::Main1: return "Main1";
  case Phase::Battle: return "Battle";
  case Phase::Main2: return "Main2";
  case Phase::End: return "End";
  }
  return "End";
}

// Minimal turn tracker.  Layer 4: only advances phases; the Duel/Engine owns
// *when* to advance.  Editing this file never touches openjoey-cards, zone/ or field/.
struct DuelProtocol {
  Phase phase = Phase::Draw;
  int turnNumber = 1;

  // First-turn restrictions (Rulebook "The player who goes first"):
  // the starting player cannot draw during their first Draw Phase and cannot
  // conduct a Battle Phase on their first turn. The Engine sets these for
  // turn 1; phases still advance through the normal walk.
  bool skipDraw = false;
  bool skipBattle = false;

  void nextPhase() {
    switch (phase) {
    case Phase::Draw:     phase = Phase::Standby; break;
    case Phase::Standby:  phase = Phase::Main1; break;
    case Phase::Main1:    phase = Phase::Battle; break;
    case Phase::Battle:   phase = Phase::Main2; break;
    case Phase::Main2:    phase = Phase::End; break;
    case Phase::End:      phase = Phase::Draw; ++turnNumber; break;
    }
  }

  bool canAct() const { // Main1 / Main2 / Battle = "possible actions" windows
    return phase == Phase::Main1 || phase == Phase::Main2 || phase == Phase::Battle;
  }

  std::string status() const {
    return "Turn " + std::to_string(turnNumber) + " — " + phaseName(phase);
  }
};

} // namespace openjoey::engine::engine
