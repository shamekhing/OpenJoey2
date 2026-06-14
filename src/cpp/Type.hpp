#pragma once
#include <cstdint>

namespace openjoey::etypes {

// Shared enum vocabulary for cards, zones, effects, and phases.
// Numeric phase values are exposed through the C ABI, so keep them stable.

// ── Card ─────────────────────────────────────────────────────────────────────

enum class card : uint8_t { Monster, Spell, Trap };

enum class location : uint8_t {
  Deck,
  Hand,
  Field,
  Graveyard,
  Banished,
  ExtraDeck,
  SideDeck,
  None
};

enum class position : uint8_t { FaceUp, FaceDown };

// ── Zone ─────────────────────────────────────────────────────────────────────

enum class zone : uint8_t {
  Monster,
  SpellTrap,
  Field,
  ExtraMonster,
  Hand,
  Deck,
  ExtraDeck,
  Graveyard,
  Banished,
  SideDeck,
  None
};

enum class orientation : uint8_t { Vertical, Horizontal };

enum class visibility : uint8_t { Visible, Limited, Restricted };

// ── Effect ───────────────────────────────────────────────────────────────────

enum class target : uint8_t { None, Own, Opponent, Any };

enum class effect : uint8_t {
  Normal,
  Continuous,
  Quick,
  Trigger,
  Counter
};

enum class spellspeed : uint8_t {
  Normal = 1,
  Quick = 2,
  Counter = 3,
};

// ── Phase ────────────────────────────────────────────────────────────────────

enum class phase : uint8_t {
  Draw = 0,
  Standby = 1,
  Main1 = 2,
  Battle = 3,
  Main2 = 4,
  End = 5,
};

} // namespace openjoey::etypes
