#pragma once
// Field.hpp pulls in the whole zone layer: enums, the interface, the
// single-card slot, the stack, and the concrete zone types it composes.
#include <array>
#include <map>
#include <memory>
#include <utility>
#include <vector>

#include "cards/Card.hpp"
#include "engine/field/zone/IZone.hpp"
#include "engine/field/zone/Zone.hpp"
#include "engine/field/zone/ZoneEnums.hpp"
#include "engine/field/zone/ZoneStack.hpp"
#include "engine/field/zone/Zones.hpp"

namespace openjoey::engine::zone {
using cards::Card;

// ─── Field
// ──────────────────────────────────────────────────────────────────── The
// physical playing field: all zones for both players.
//
//   monsterZones[p][0..4]      — 5 main monster zones per player
//   spellTrapZones[p][0..4]    — 5 spell/trap zones; [p][0] and [p][4] are
//                                also Pendulum Zones
//   fieldZones[p]              — 1 field spell zone per player
//   extraMonsterZones[0..1]    — 2 EMZs shared on the mat
//   handZones[p]               — player hand
//   deckZones[p]               — player main deck
//   extraDeckZones[p]          — player extra deck
//   graveyardZones[p]          — player graveyard
//   banishedZones[p]           — player banished pile
//   sideDeckZones[p]           — player side deck

class Field {
   public:
    static constexpr int PLAYERS = 2;
    static constexpr int MONSTER_ZONES = 5;
    static constexpr int ST_ZONES = 5;
    static constexpr int EMZ_COUNT = 2;

    std::array<std::array<Zone_Monster, MONSTER_ZONES>, PLAYERS> monsterZones;
    std::array<std::array<Zone_SpellTrap, ST_ZONES>, PLAYERS> spellTrapZones;
    std::array<Zone_Field, PLAYERS> fieldZones;
    std::array<Zone_ExtraMonster, EMZ_COUNT> extraMonsterZones;

    std::array<ZoneStack_Hand, PLAYERS> handZones;
    std::array<ZoneStack_Deck, PLAYERS> deckZones;
    std::array<ZoneStack_ExtraDeck, PLAYERS> extraDeckZones;
    std::array<ZoneStack_Graveyard, PLAYERS> graveyardZones;
    std::array<ZoneStack_Banished, PLAYERS> banishedZones;
    std::array<ZoneStack_SideDeck, PLAYERS> sideDeckZones;

    // Engine-spawned tokens the mat owns: they exist only while on the field
    // and cease to exist the moment they leave it (see action/builtins).

    std::vector<std::unique_ptr<Card>> tokens;

    // Clear all on-field zones (does not clear decks/hands/GY/banished).
    void clearField();

    // Find the first empty monster zone for a player; returns -1 if full.
    int firstEmptyMonsterZone(int player) const;

    // Find the first empty spell/trap zone for a player; returns -1 if full.
    int firstEmptySpellTrapZone(int player) const;

    // Find the first occupied monster zone for a player; returns -1 if none.
    int firstOccupiedMonsterZone(int player) const;

    // Count monsters on field (main zones + EMZ controlled by player).
    int countMonsters(int player) const;

    // Return the first available Extra Monster Zone index (0 or 1), or -1.
    int firstEmptyExtraMonsterZone() const;

    // Snapshot/undo support: swap token card pointers after a deep token copy.
    void remapPointers(const std::map<Card *, Card *> &m);

    // ── Semantic lookup (the one place that knows the arrays) ────────────────
    // The zone at (player, type, slot) — nullptr if out of range. Slot indexes
    // match the arrays (0..4); for single/stack zones the slot is ignored.
    IZone *zoneAt(int player, ZoneType t, int i) {
        if (player < 0 || player >= PLAYERS)
            return nullptr;
        switch (t) {
            case ZoneType::Monster:
                return (i >= 0 && i < MONSTER_ZONES)
                           ? &monsterZones[player][i]
                           : nullptr;
            case ZoneType::SpellTrap:
                return (i >= 0 && i < ST_ZONES)
                           ? &spellTrapZones[player][i]
                           : nullptr;
            case ZoneType::Field:
                return &fieldZones[player];
            case ZoneType::ExtraMonster:
                return (i >= 0 && i < EMZ_COUNT)
                           ? &extraMonsterZones[i]
                           : nullptr;
            case ZoneType::Hand:
                return &handZones[player];
            case ZoneType::Deck:
                return &deckZones[player];
            case ZoneType::ExtraDeck:
                return &extraDeckZones[player];
            case ZoneType::Graveyard:
                return &graveyardZones[player];
            case ZoneType::Banished:
                return &banishedZones[player];
            case ZoneType::SideDeck:
                return &sideDeckZones[player];
        }
        return nullptr;
    }

    // The main monster zone holding `c`, or nullptr. (Classic scope: the Extra
    // Monster Zone is not consulted.)
    Zone_Monster *monsterZoneOf(Card *c) {
        if (!c)
            return nullptr;
        for (int p = 0; p < PLAYERS; ++p)
            for (int z = 0; z < MONSTER_ZONES; ++z)
                if (monsterZones[p][z].contains(c))
                    return &monsterZones[p][z];
        return nullptr;
    }

    const IZone *zoneAt(int player, ZoneType t, int i) const {
        return const_cast<Field *>(this)->zoneAt(player, t, i);
    }
    const Zone_Monster *monsterZoneOf(const Card *c) const {
        return const_cast<Field *>(this)->monsterZoneOf(const_cast<Card *>(c));
    }

    // ── Location lookup ────────────────────────────────────────────────────
    // Find which zone (and which player index) currently holds `c`.
    // Returns {nullptr, -1} if the card is in no zone on the field.
    // Query-only: never mutates a zone.  Layer 3 (field) owns this because it
    // must iterate the whole mat; layer 2 (zone) only knows its own contents.
    std::pair<IZone *, int> findCard(Card *c);
    std::pair<const IZone *, int> findCard(const Card *c) const;
};

inline void Field::clearField() {
    for (int p = 0; p < PLAYERS; ++p) {
        for (int z = 0; z < MONSTER_ZONES; ++z) {
            monsterZones[p][z].reset();
            spellTrapZones[p][z].reset();
        }
        fieldZones[p].remove();
    }
}

inline int Field::firstEmptyMonsterZone(int player) const {
    for (int z = 0; z < MONSTER_ZONES; ++z)
        if (monsterZones[player][z].isEmpty())
            return z;
    return -1;
}

inline int Field::firstEmptySpellTrapZone(int player) const {
    for (int z = 0; z < ST_ZONES; ++z)
        if (spellTrapZones[player][z].isEmpty())
            return z;
    return -1;
}

inline int Field::firstOccupiedMonsterZone(int player) const {
    for (int z = 0; z < MONSTER_ZONES; ++z)
        if (!monsterZones[player][z].isEmpty())
            return z;
    return -1;
}

inline int Field::countMonsters(int player) const {
    int n = 0;
    for (int z = 0; z < MONSTER_ZONES; ++z)
        if (!monsterZones[player][z].isEmpty())
            ++n;
    for (int z = 0; z < EMZ_COUNT; ++z) {
        Card *c = extraMonsterZones[z].peek();
        if (c && c->state.controller == player)
            ++n;
    }
    return n;
}

inline int Field::firstEmptyExtraMonsterZone() const {
    for (int z = 0; z < EMZ_COUNT; ++z)
        if (extraMonsterZones[z].isEmpty())
            return z;
    return -1;
}

// ── Snapshot/undo support: swap token card pointers after a deep token copy ─
// External (deck-owned) cards keep their addresses across a Duel clone, so
// only engine-owned tokens need remapping.
inline void Field::remapPointers(const std::map<Card *, Card *> &m) {
    if (m.empty())
        return;
    auto sw = [&](IZone *z) {
        if (!z)
            return;
        for (auto &[from, to] : m)
            z->replacePtr(from, to);
    };
    for (int p = 0; p < PLAYERS; ++p) {
        for (auto &z : monsterZones[p]) sw(&z);
        for (auto &z : spellTrapZones[p]) sw(&z);
        sw(&fieldZones[p]);
        sw(&handZones[p]);
        sw(&deckZones[p]);
        sw(&extraDeckZones[p]);
        sw(&graveyardZones[p]);
        sw(&banishedZones[p]);
        sw(&sideDeckZones[p]);
    }
    for (auto &z : extraMonsterZones) sw(&z);
}

// ── findCard ───────────────────────────────────────────────────────────────
inline std::pair<IZone *, int> Field::findCard(Card *c) {
    if (!c) return {nullptr, -1};
    for (int p = 0; p < PLAYERS; ++p) {
        for (int i = 0; i < MONSTER_ZONES; ++i)
            if (monsterZones[p][i].contains(c)) return {static_cast<IZone *>(&monsterZones[p][i]), p};
        for (int i = 0; i < ST_ZONES; ++i)
            if (spellTrapZones[p][i].contains(c)) return {static_cast<IZone *>(&spellTrapZones[p][i]), p};
        if (fieldZones[p].contains(c)) return {static_cast<IZone *>(&fieldZones[p]), p};
        if (handZones[p].contains(c)) return {static_cast<IZone *>(&handZones[p]), p};
        if (deckZones[p].contains(c)) return {static_cast<IZone *>(&deckZones[p]), p};
        if (extraDeckZones[p].contains(c)) return {static_cast<IZone *>(&extraDeckZones[p]), p};
        if (graveyardZones[p].contains(c)) return {static_cast<IZone *>(&graveyardZones[p]), p};
        if (banishedZones[p].contains(c)) return {static_cast<IZone *>(&banishedZones[p]), p};
        if (sideDeckZones[p].contains(c)) return {static_cast<IZone *>(&sideDeckZones[p]), p};
    }
    for (int z = 0; z < EMZ_COUNT; ++z)
        if (extraMonsterZones[z].contains(c)) return {static_cast<IZone *>(&extraMonsterZones[z]), -1};
    return {nullptr, -1};
}

inline std::pair<const IZone *, int> Field::findCard(const Card *c) const {
    // Delegate to the non-const overload via const_cast on `this` (the lookup is
    // read-only). Casting `this` — not the argument — avoids infinite recursion:
    // `findCard(const_cast<Card*>(c))` here would re-resolve to this same const
    // overload (const `this` is the better match) and overflow the stack.
    auto [z, p] = const_cast<Field *>(this)->findCard(const_cast<Card *>(c));
    return {z, p};
}

}  // namespace openjoey::engine::zone
