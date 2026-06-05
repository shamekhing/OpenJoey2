#pragma once
#include <cstdint>

namespace openjoey {

// ── Card ─────────────────────────────────────────────────────────────────────

enum class enum_card : uint8_t { Monster, Spell, Trap };

// ── Zone ─────────────────────────────────────────────────────────────────────

enum class enum_zone : uint8_t {
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

enum class enum_orientation : uint8_t { Vertical, Horizontal };

enum class enum_visibility : uint8_t { Visible, Limited, Restricted };

// ── Effect ───────────────────────────────────────────────────────────────────

enum class enum_target : uint8_t { None, Own, Opponent, Any };

enum class enum_effect : uint8_t {
  Normal,
  Continuous,
  Quick,
  Trigger,
  Counter
};

enum class enum_spellspeed : uint8_t {
  Normal = 1,
  Quick = 2,
  Counter = 3,
};

// ── Phase ────────────────────────────────────────────────────────────────────

enum class enum_phase : uint8_t {
  Draw = 0,
  Standby = 1,
  Main1 = 2,
  Battle = 3,
  Main2 = 4,
  End = 5,
};

} // namespace openjoey
