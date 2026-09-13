#include "engine/field/Field.hpp"

namespace openjoey::engine::zone {

Field::Field()
    : fieldZones{Zone(ZoneType::Field), Zone(ZoneType::Field)},
      extraMonsterZones{Zone(ZoneType::ExtraMonster), Zone(ZoneType::ExtraMonster)},
      monsterZones{ZoneSpread(MONSTER_ZONES, ZoneType::Monster), ZoneSpread(MONSTER_ZONES, ZoneType::Monster)},
      spellTrapZones{ZoneSpread(ST_ZONES, ZoneType::SpellTrap), ZoneSpread(ST_ZONES, ZoneType::SpellTrap)},
      handZones{ZoneStack(ZoneType::Hand), ZoneStack(ZoneType::Hand)},
      deckZones{ZoneStack(ZoneType::Deck), ZoneStack(ZoneType::Deck)},
      extraDeckZones{ZoneStack(ZoneType::ExtraDeck), ZoneStack(ZoneType::ExtraDeck)},
      graveyardZones{ZoneStack(ZoneType::Graveyard), ZoneStack(ZoneType::Graveyard)},
      banishedZones{ZoneStack(ZoneType::Banished), ZoneStack(ZoneType::Banished)},
      sideDeckZones{ZoneStack(ZoneType::SideDeck), ZoneStack(ZoneType::SideDeck)} {}

void Field::reset() {
    for (int player = 0; player < PLAYERS; ++player) {
        monsterZones[player].reset();
        spellTrapZones[player].reset();
        fieldZones[player].reset();
    }
    for (auto &zone : extraMonsterZones) zone.reset();
}

void Field::remapPointers(const std::map<Card *, Card *> &mapping) {
    if (mapping.empty()) return;
    auto remap = [&](IZone *zone) {
        if (!zone) return;
        for (const auto &[from, to] : mapping) zone->replacePtr(from, to);
    };
    for (int player = 0; player < PLAYERS; ++player) {
        remap(&monsterZones[player]);
        remap(&spellTrapZones[player]);
        remap(&fieldZones[player]);
        remap(&handZones[player]);
        remap(&deckZones[player]);
        remap(&extraDeckZones[player]);
        remap(&graveyardZones[player]);
        remap(&banishedZones[player]);
        remap(&sideDeckZones[player]);
    }
    for (auto &zone : extraMonsterZones) remap(&zone);
}

std::pair<IZone *, int> Field::findCard(Card *card) {
    if (!card) return {nullptr, -1};
    const int preferredPlayer = card->state.controller;
    const int firstPlayer = preferredPlayer >= 0 && preferredPlayer < PLAYERS ? preferredPlayer : 0;
    for (int pass = 0; pass < PLAYERS; ++pass) {
        const int player = pass == 0 ? firstPlayer : 1 - firstPlayer;
        for (int slot = 0; slot < MONSTER_ZONES; ++slot)
            if (monsterZones[player][slot].contains(card)) return {&monsterZones[player][slot], player};
        for (int slot = 0; slot < ST_ZONES; ++slot)
            if (spellTrapZones[player][slot].contains(card)) return {&spellTrapZones[player][slot], player};
        if (fieldZones[player].contains(card)) return {&fieldZones[player], player};
        if (handZones[player].contains(card)) return {&handZones[player], player};
        if (deckZones[player].contains(card)) return {&deckZones[player], player};
        if (extraDeckZones[player].contains(card)) return {&extraDeckZones[player], player};
        if (graveyardZones[player].contains(card)) return {&graveyardZones[player], player};
        if (banishedZones[player].contains(card)) return {&banishedZones[player], player};
        if (sideDeckZones[player].contains(card)) return {&sideDeckZones[player], player};
    }
    for (int slot = 0; slot < EMZ_COUNT; ++slot)
        if (extraMonsterZones[slot].contains(card)) return {&extraMonsterZones[slot], -1};
    return {nullptr, -1};
}

std::pair<const IZone *, int> Field::findCard(const Card *card) const {
    auto [zone, player] = const_cast<Field *>(this)->findCard(const_cast<Card *>(card));
    return {zone, player};
}

}  // namespace openjoey::engine::zone
