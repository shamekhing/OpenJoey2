#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "action/ActionSpec.hpp"
#include "cards/CardEnums.hpp"

namespace openjoey::cards {
// ─────────────────────────────────────────────────────────────────────────────
// ────────────────────────────── CARD CORE ────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
struct Card;  // fwd: CardState holds non-owning pointers to cards

// CardDef: immutable per card id; populated by parser.
// Method declarations only — definitions follow below.
//
// Definition order (callee before caller):
//   hasAttribute ← hasAttributes ← isExtraDeckMonster
//   hasAttribute ← isMonster / isSpell / isTrap
struct CardDef {
    uint32_t id = 0;
    std::string name;
    std::string description;

    int atk = 0, def = 0, level = 0;

    std::vector<Attribute> attributes;  // all attributes (for filtering, etc.)

    // ── attribute membership ──
    bool hasAttribute(Attribute a) const;
    bool hasAttributes(const std::vector<Attribute> &as, bool any = false) const;

    // ── identity queries (pure CardDef concerns) ──
    bool isMonster() const;
    bool isSpell() const;
    bool isTrap() const;
    bool isExtraDeckMonster() const;
};

// CardState: mutated by the engine.
// (no methods, only data members)
struct CardState {
    int owner = -1;       // player index
    int controller = -1;  // player index (may differ from owner)

    bool setThisTurn = false;
    bool placedThisTurn = false;

    std::vector<Card *> equippedCards;
    std::vector<Card *> xyzMaterials;
    std::map<std::string, int> counters;

    Card *equipTarget = nullptr;
    int bonusAtk = 0, bonusDef = 0, atkMod = 0, defMod = 0;

    bool isToken = false;
};

// Card: the one and only.
// Inherits CardDef, adds a CardState instance.
// Method declarations only — definitions follow below.
//
// Definition order (callee before caller):
//   operator== ← operator!=
struct Card : CardDef {
    CardState state;

    // Equality is identity-by-id: equal iff both have a non-zero cardId match.
    bool operator==(const Card &other) const;
    bool operator!=(const Card &other) const;

    // ── effective battle stats (original + modifiers, never below 0) ─────────
    int effectiveAtk() const;
    int effectiveDef() const;

    // ── presentation helpers (raylib-free string formatting) ──────────────────
    std::string cardTypeTag() const;
    std::string statLine() const;
    std::string shortStat() const;
};

}  // namespace openjoey::cards
