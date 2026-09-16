#ifndef OPENJOEY_CARDS_CARDS_H_
#define OPENJOEY_CARDS_CARDS_H_
// ── openjoey::cards — public API umbrella (raylib-free) ──────────────────────
// One include for the card domain. Cards are identity (CardDef) + placement
// state (CardState); parsing, database and comparators live alongside.
#include "cards/card.hpp"
#include "cards/card_compare.hpp"
#include "cards/card_database.hpp"
#include "cards/card_enums.hpp"
#include "cards/card_parser.hpp"
#endif  // OPENJOEY_CARDS_CARDS_H_
