#pragma once
// ── openjoey::cards — public API umbrella (raylib-free) ──────────────────────
// One include for the card domain. Cards are identity (CardDef) + placement
// state (CardState); parsing, database and comparators live alongside.
#include "cards/Card.hpp"
#include "cards/CardCompare.hpp"
#include "cards/CardDatabase.hpp"
#include "cards/CardEnums.hpp"
#include "cards/CardParser.hpp"