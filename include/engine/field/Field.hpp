#pragma once
// Field.hpp pulls in the whole zone layer: enums, the interface, the
// single-card slot, the stack, and the concrete zone types it composes.
#include <array>
#include <map>
#include <memory>
#include <string>
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

// ─── Field ───────────────────────────────────────────────────────────────────
// The physical playing field: all zones for both players.
//   monsterZones[p][0..4]      — 5 main monster zones per player
//   spellTrapZones[p][0..4]    — 5 spell/trap zones; [p][0]/[p][4] also Pend.
//   fieldSpellZones[p]              — 1 field spell zone per player
//   handZones[p] / deckZones[p] / extraDeckZones[p] / graveyardZones[p] /
//   banishedZones[p] / sideDeckZones[p]
//
// Field's job is SEARCH + ACCESS only: return zones (as a group, as a whole, or
// individually), and locate a zone/card by pointer or name. It does NOT edit.
// Editing lives in the Action layer and reaches in through the public members
// (or the accessors below) calling IZone::put / remove itself.
//
// Browsing API:
//   * get() / get(zt) / get(player) / get(player, zt)  -> whole / type / owner / one
//   * findCard(Card*) / findCard(const Card*) const    -> {zone, player} holding a card
//   * zoneOf(Card*)                                -> zone holding a card
//   * zonesOf(const string& name)                  -> zones holding a card named "name"
//   * cardIn(zt, player, slot=-1)                  -> top card of a stack / cell card
//   * cards(zt, player) / allCards()               -> every card in a zone / on the mat
//   * countIn(zt, player)                          -> number of cards in a zone
class Field {
   public:
    static constexpr int PLAYERS = 2;
    static constexpr int MONSTER_ZONES = 5;
    static constexpr int ST_ZONES = 5;

    Field();

    // ── Zone storage (public so Actions can edit in place; Field only browses)
    std::array<Zone, PLAYERS> fieldSpellZones;
    std::array<ZoneSpread, PLAYERS> monsterZones;
    std::array<ZoneSpread, PLAYERS> spellTrapZones;
    std::array<ZoneStack, PLAYERS> handZones;
    std::array<ZoneStack, PLAYERS> deckZones;
    std::array<ZoneStack, PLAYERS> extraDeckZones;
    std::array<ZoneStack, PLAYERS> graveyardZones;
    std::array<ZoneStack, PLAYERS> banishedZones;
    std::array<ZoneStack, PLAYERS> sideDeckZones;
    std::vector<std::unique_ptr<Card>> tokens;   // engine-owned, on-field only

    // ── Browsing ─────────────────────────────────────────────────────────────
    // Whole mat, or a single type+owner's zones.
    std::vector<IZone*> get(int player = PLAYERS);
    std::vector<IZone*> get(ZoneType zt);
    IZone* get(int player, ZoneType zt);
    
    // ── Search ───────────────────────────────────────────────────────────────
    std::pair<IZone*, int> findCard(Card* c);              // returns {zone, player}
    std::pair<const IZone*, int> findCard(const Card* c) const;
    IZone* zoneOf(Card* c);                                // just the zone holding a card
    Zone* monsterZoneOf(Card* c);                          // the monster cell holding c, or nullptr
    const Zone* monsterZoneOf(const Card* c) const;        // const overload for guards
    std::vector<IZone*> zonesOf(const std::string& name);   // zone holding a card named `name`

    // ── Cards ────────────────────────────────────────────────────────────────
    Card* cardIn(ZoneType zt, int player, int slot = -1);  // top card / cell card (nullptr..)
    std::vector<Card*> cards(ZoneType zt, int player);     // all cards in a zone
    std::vector<Card*> allCards();                         // every card in zones + tokens

    // ── Counts ───────────────────────────────────────────────────────────────
    int countIn(ZoneType zt, int player);

    // ── Lifecycle ────────────────────────────────────────────────────────────
    void reset();                                           // clear on-field zones only
    void remapPointers(const std::map<Card*, Card*>& m);    // undo-snap token remap
};
}  // namespace openjoey::engine::zone
