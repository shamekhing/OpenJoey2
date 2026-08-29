#pragma once
#include "field/Field.hpp"
#include "duel/TurnManager.hpp"
#include "duel/Chain.hpp"
#include "card/Card.hpp"
#include <array>

namespace openjoey {

// ── Duel (layer 4: depends on field/ zone/ card/) ────────────────────────────
// Owns the shared game state: Life Points, whose turn it is, the Field, the
// TurnManager and an open Chain.  It holds no raylib types — the engine is
// UI-free and unit-testable.  Editing duel/ never reaches into field/ or
// below.
struct Duel {
  zone::Field field;        // the mat of zones (layer 3)
  TurnManager turn;         // phase progression (layer 4)
  Chain chain;              // open chain of activations (layer 4)

  std::array<int, 2> lp{8000, 8000};
  int turnPlayer = 0;       // who's taking the turn
  int activePlayer = 0;     // who has priority / is acting

  // True if a Phase is "open" for action (Rulebook p.30 Main phases / Battle).
  bool canAct() const { return turn.canAct(); }

  // Convenience: is the controller of `c` player `p`?  (-1 controller is "none".)
  bool controls(const Card *c, int p) const {
    return c && c->controller == p;
  }
};

} // namespace openjoey
