#pragma once
#include <cstdint>

namespace openjoey {

// ── Card ─────────────────────────────────────────────────────────────────────

enum class CardType : uint8_t { Monster, Spell, Trap };

enum class Position : uint8_t { FaceUp, FaceDown };

enum class Location : uint8_t {
  Hand,
  Deck,
  ExtraDeck,
  Field,
  Graveyard,
  Banished,
  None
};

// ── Zone ─────────────────────────────────────────────────────────────────────

enum class ZoneType : uint8_t {
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
  None,
};

enum class Orientation : uint8_t {
  Vertical,
  Horizontal,
};

enum class Visibility : uint8_t {
  Visible,
  Limited,
  Restricted,
};

// ── Effect ───────────────────────────────────────────────────────────────────

enum class Target : uint8_t {
  None,
  Own,
  Opponent,
  Any,
};

enum class EffectType : uint8_t {
  Normal,
  Continuous,
  Quick,
  Trigger,
  Counter,
};

enum class SpellSpeed : uint8_t {
  Normal  = 1,
  Quick   = 2,
  Counter = 3,
};

} // namespace openjoey

namespace openjoey::game {

// ── Phase ────────────────────────────────────────────────────────────────────

enum class Phase : uint8_t {
  Draw    = 0,
  Standby = 1,
  Main1   = 2,
  Battle  = 3,
  Main2   = 4,
  End     = 5,
};

} // namespace openjoey::game
