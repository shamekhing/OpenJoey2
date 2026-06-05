#pragma once
#include <cstdint>

namespace openjoey {

// ── Card ─────────────────────────────────────────────────────────────────────

enum class enum_card : uint8_t { Monster, Spell, Trap };

enum class enum_location : uint8_t {
  Deck,
  Hand,
  Field,
  Graveyard,
  Banished,
  ExtraDeck,
  SideDeck,
  None
};

enum class enum_position : uint8_t {
  FaceUp,
  FaceDown
};

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

using CardType = enum_card;
using Location = enum_location;
using Position = enum_position;
using ZoneType = enum_zone;
using Orientation = enum_orientation;
using Visibility = enum_visibility;
using Phase = enum_phase;
using Target = enum_target;
using EffectType = enum_effect;
using SpellSpeed = enum_spellspeed;

} // namespace openjoey
