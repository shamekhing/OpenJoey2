#pragma once
#include <algorithm>
#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include "action/ActionSpec.hpp"
#include "cards/CardEnums.hpp"

namespace openjoey::cards {

struct Card;  // fwd: CardState holds non-owning pointers to cards

// ── CardDef — what the card IS (immutable per card id; populated by parser) ──
struct CardDef {
    uint32_t id = 0;
    std::string name;
    std::string description;

    int atk = 0, def = 0, level = 0;

    std::vector<Attribute> attributes;  // all attributes (for filtering, etc.)

    bool hasAttribute(Attribute a) const { return std::find(attributes.begin(), attributes.end(), a) != attributes.end(); }
    bool hasAttributes(const std::vector<Attribute> &as, bool any = false) const {
        for (const auto &a : as)
            if (hasAttribute(a) == any) return any;
        return !any;
    }

    // ── identity queries (pure CardDef concerns) ─────────────────────────────
    bool isMonster() const { return hasAttribute(Attribute::Monster); }
    bool isSpell() const { return hasAttribute(Attribute::Spell); }
    bool isTrap() const { return hasAttribute(Attribute::Trap); }
    bool isExtraDeckMonster() const { return hasAttributes({Attribute::Fusion, Attribute::Synchro, Attribute::Xyz}, true); }
};

// ── CardState  ──────────────────────────────(mutated by the engine) ───────

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

// ── Card — the one-and-only card type (identity inherited, state composed) ──
struct Card : CardDef {
    CardState state;

    // Equality is identity-by-id: equal iff both have a non-zero cardId match.
    bool operator==(const Card &other) const { return id != 0 && id == other.id; }
    bool operator!=(const Card &other) const { return !(*this == other); }

    // ── effective battle stats (original + modifiers, never below 0) ─────────
    int effectiveAtk() const { return std::max(0, atk + state.atkMod); }
    int effectiveDef() const { return std::max(0, def + state.defMod); }

    // ── presentation helpers (raylib-free string formatting) ──────────────────
    std::string cardTypeTag() const { return isMonster() ? "[MON]" : isSpell() ? "[SPL]"
                                                                 : isTrap()    ? "[TRP]"
                                                                               : "[UNK]"; }
    std::string statLine() const { return isMonster() ? "Level " + std::to_string(level) + "  ATK " + std::to_string(atk) + "  DEF " + std::to_string(def) : ""; }
    std::string shortStat() const { return isMonster() ? "L" + std::to_string(level) + " " + std::to_string(atk) + "/" + std::to_string(def) : ""; }
};

}  // namespace openjoey::cards
