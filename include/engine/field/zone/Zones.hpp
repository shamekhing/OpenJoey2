#pragma once

#include "engine/field/zone/Zone.hpp"
#include "engine/field/zone/ZoneStack.hpp"
#include "engine/field/zone/ZoneSpread.hpp"

namespace openjoey::engine::zone {

// Compatibility aliases. Zone behavior is generic; actions select rules by
// ZoneType instead of relying on specialized zone classes.
using Zone_Monster = Zone;
using Zone_SpellTrap = Zone;
using Zone_Field = Zone;
using ZoneStack_Hand = ZoneStack;
using ZoneStack_Deck = ZoneStack;
using ZoneStack_ExtraDeck = ZoneStack;
using ZoneStack_Graveyard = ZoneStack;
using ZoneStack_Banished = ZoneStack;
using ZoneStack_SideDeck = ZoneStack;

}  // namespace openjoey::engine::zone
