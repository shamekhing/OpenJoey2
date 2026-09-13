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
#include "engine/field/zone/ZoneSpread.hpp"
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


    Field();

        std::array<Zone, PLAYERS> fieldZones;
        std::array<Zone, EMZ_COUNT> extraMonsterZones;
        std::array<ZoneSpread, PLAYERS> monsterZones;
        std::array<ZoneSpread, PLAYERS> spellTrapZones;
        std::array<ZoneStack, PLAYERS> handZones;
        std::array<ZoneStack, PLAYERS> deckZones;
        std::array<ZoneStack, PLAYERS> extraDeckZones;
        std::array<ZoneStack, PLAYERS> graveyardZones;
        std::array<ZoneStack, PLAYERS> banishedZones;
        std::array<ZoneStack, PLAYERS> sideDeckZones;


    // Engine-spawned tokens the mat owns: they exist only while on the field
    // and cease to exist the moment they leave it (see action/builtins).

    std::vector<std::unique_ptr<Card>> tokens;

    // Clear all on-field zones (does not clear decks/hands/GY/banished).
    void reset();
    
    // Snapshot/undo support: swap token card pointers after a deep token copy.
    void remapPointers(const std::map<Card *, Card *> &m);

    // ── Location lookup ────────────────────────────────────────────────────
    // Find which zone (and which player index) currently holds `c`.
    // Returns {nullptr, -1} if the card is in no zone on the field.
    // Query-only: never mutates a zone.  Layer 3 (field) owns this because it
    // must iterate the whole mat; layer 2 (zone) only knows its own contents.
    std::pair<IZone *, int> findCard(Card *c);
    std::pair<const IZone *, int> findCard(const Card *c) const;
};

}  // namespace openjoey::engine::zone
