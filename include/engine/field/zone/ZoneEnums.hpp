#pragma once
// ── Zone enums (openjoey::zone) ──────────────────────────────────────────────
// Shared vocabulary for the zone layer. Header-only, raylib-free.
// Split out of the old monolithic zone/Zone.hpp so consumers that only need
// the enums — UI grids, panels, label tables — can include just this header.
// The class split mirrors the openjoey-cards layout (CardEnums.hpp / Card.hpp
// / CardEffect.hpp): one concept per header.

#include <cstdint>

namespace openjoey::engine::zone {
using cards::Card;

// Every zone on the mat (10). Single-card slots and stacks both use this.
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
};

// Battle position of a card on the field (ATK = vertical, DEF = horizontal).
enum class Orientation : uint8_t {
    Vertical,
    Horizontal,
};

// Entitlement — who may look at a card in this zone:
//   Visible    — both players
//   Limited    — exactly one player (the owner/controller knows)
//   Restricted — neither player (e.g. the deck's order)
enum class Visibility : uint8_t {
    Visible,
    Limited,
    Restricted,
};

}  // namespace openjoey::engine::zone