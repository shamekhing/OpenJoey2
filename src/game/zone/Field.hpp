#pragma once
#include "Zone.hpp"
#include "card/Card.hpp"
#include <array>

namespace openjoey::zone {

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
    if (c && c->controller == player)
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

} // namespace openjoey::zone
