#pragma once
#include <cstdint>

namespace openjoey::cards {

// Card classification attributes: frame family, attribute, mechanic.
// NOTE: one enumerator per name (the pre-restructure copy had duplicates).
enum class Attribute : uint8_t { None, Monster, Spell, Trap, Dark, Light, Earth, Water, Fire, Wind, Divine, Normal, Effect, Fusion, Ritual, Synchro, Xyz, Equip, Continuous, Field, QuickPlay, Counter };

}  // namespace openjoey::cards
