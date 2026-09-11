#pragma once
#include <cstdint>

namespace openjoey::cards {
// ─────────────────────────────────────────────────────────────────────────────
// ────────────────────────────── CARD ENUMS ───────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// Attribute: the COMPLETE set of card classifiers docs/cards.json provides.
//  * elemental attribute -> "attribute"            DARK/LIGHT/EARTH/WATER/FIRE/WIND/DIVINE
//  * frame family        -> "type"/"frameType"     Monster/Spell/Trap/Skill/Token
//  * monster sub-type    -> "type"/"typeline"      Normal/Effect/Fusion/Ritual/Synchro/Xyz
//                                                   Link/Flip/Tuner/Spirit/Gemini/Union/Toon/Pendulum/Illusion
//  * spell/trap icon     -> "race" (non-monster)   Equip/Continuous/Field/Quick-Play/Counter
//  * banlist status      -> "banlist_info"         Limited/Semi-Limited/Forbidden/Unlimited
//  * link markers        -> "linkmarkers"          Left/Right/Top/Bottom + diagonals
//  * monster race        -> "race" (monster)       Dragon/Spellcaster/.../Cyberse/Illusion
enum class Attribute : uint8_t {
    // no attribute, or unrecognised string
    None,
    // elemental attribute
    Dark, Light, Earth, Water, Fire, Wind, Divine,
    // frame family / category
    Monster, Spell, Trap, Skill, Token,
    // monster sub-types
    Normal, Effect, Fusion, Ritual, Synchro, Xyz, Link,
    Flip, Tuner, Spirit, Gemini, Union, Toon, Pendulum, Illusion,
    // spell/trap icon
    Equip, Continuous, Field, QuickPlay, Counter,
    // banlist status
    Limited, Forbidden, SemiLimited, Unlimited,
    // link markers
    LinkMarkerLeft, LinkMarkerRight, LinkMarkerTop, LinkMarkerBottom,
    LinkMarkerTopLeft, LinkMarkerTopRight, LinkMarkerBottomLeft, LinkMarkerBottomRight,
    // monster race
    Dragon, Spellcaster, Warrior, Fairy, Fiend, Zombie,
    Machine, Aqua, Pyro, Rock, WingedBeast, Plant,
    Insect, Thunder, Beast, BeastWarrior, Dinosaur, Fish,
    SeaSerpent, Reptile, Psychic, DivineBeast, CreatorGod, Wyrm, Cyberse,
};

// ─────────────────────────────────────────────────────────────────────────────
// ──────── STANDALONE HELPERS (callee before caller) ──────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

std::string normaliseString(const std::string &str);
Attribute attribute_from_string(const std::string &str);
} // namespace openjoey::cards