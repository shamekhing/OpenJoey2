#include "engine/field/Field.hpp"

namespace openjoey::engine::zone {

Field::Field()
    : fieldSpellZones{Zone(ZoneType::Field), Zone(ZoneType::Field)},
      monsterZones{ZoneSpread(MONSTER_ZONES, ZoneType::Monster), ZoneSpread(MONSTER_ZONES, ZoneType::Monster)},
      spellTrapZones{ZoneSpread(ST_ZONES, ZoneType::SpellTrap), ZoneSpread(ST_ZONES, ZoneType::SpellTrap)},
      handZones{ZoneStack(ZoneType::Hand), ZoneStack(ZoneType::Hand)},
      deckZones{ZoneStack(ZoneType::Deck), ZoneStack(ZoneType::Deck)},
      extraDeckZones{ZoneStack(ZoneType::ExtraDeck), ZoneStack(ZoneType::ExtraDeck)},
      graveyardZones{ZoneStack(ZoneType::Graveyard), ZoneStack(ZoneType::Graveyard)},
      banishedZones{ZoneStack(ZoneType::Banished), ZoneStack(ZoneType::Banished)},
      sideDeckZones{ZoneStack(ZoneType::SideDeck), ZoneStack(ZoneType::SideDeck)} {}

void Field::reset() {
    for (auto z : get())
        z->reset();
}

void Field::remapPointers(const std::map<Card*, Card*>& mapping) {
    if (mapping.empty()) return;
    auto remap = [&](IZone* zone) {
        if (!zone) return;
        for (const auto& [from, to] : mapping) zone->replacePtr(from, to);
    };
    for (auto z : get())
        remap(z);
}

std::pair<IZone*, int> Field::findCard(Card* card) {
    if (!card)
        return {nullptr, -1};
    for (int p = 0; p < PLAYERS; p++) {
        for (auto z : get(p))
            if (z->contains(card))
                return {z, p};
    }
    return {nullptr, -1};
}

std::pair<const IZone*, int> Field::findCard(const Card* card) const {
    const auto [zone, player] = const_cast<Field*>(this)->findCard(const_cast<Card*>(card));
    return {zone, player};
}

std::vector<IZone*> Field::get(int player) {
    std::vector<IZone*> out;

    auto push = [&](int p){
        out.push_back(&fieldSpellZones[p]);
        out.push_back(&monsterZones[p]);
        out.push_back(&spellTrapZones[p]);
        out.push_back(&handZones[p]);
        out.push_back(&deckZones[p]);
        out.push_back(&extraDeckZones[p]);
        out.push_back(&graveyardZones[p]);
        out.push_back(&banishedZones[p]);
        out.push_back(&sideDeckZones[p]);
    };

    if (player >= 0 && player < PLAYERS)
        push(player);
    else for (int p = 0; p < PLAYERS; ++p)
        push(p);
    return out;
}

std::vector<IZone*> Field::get(ZoneType zt) {
    std::vector<IZone*> out;
    for (auto z : get()) {
        if (z->type() == zt)
            out.push_back(z);
    }
    return out;
}

IZone* Field::get(int p, ZoneType zt) {
    if (p < 0 || p >= PLAYERS) return nullptr;
    auto zones = get(zt);
    return p < static_cast<int>(zones.size()) ? zones[p] : nullptr;
}

IZone* Field::zoneOf(Card* c) { return findCard(c).first; }

std::vector<IZone*> Field::zonesOf(const std::string& name) {
    std::vector<IZone*> out;
    for (IZone* z : get())
        if (z->contains(name))
            out.push_back(z);
    return out;
}

Card* Field::cardIn(ZoneType zt, int player, int slot) {
    IZone* z = get(player, zt);
    return z ? z->peek(slot) : nullptr;
}

std::vector<Card*> Field::cards(ZoneType zt, int player) {
    std::vector<Card*> out;
    IZone* z = get(player, zt);
    if (!z) return out;
    for (int i = 0, n = z->count(); i < n; ++i)
        out.push_back(z->peek(i));
    return out;
}

std::vector<Card*> Field::allCards() {
    std::vector<Card*> out;
    for (IZone* z : get())
        for (int i = 0, n = z->count(); i < n; ++i)
            out.push_back(z->peek(i));
    for (auto& t : tokens)
        if (t) out.push_back(t.get());
    return out;
}

int Field::countIn(ZoneType zt, int player) {
    IZone* z = get(player, zt);
    return z ? z->count() : 0;
}

}  // namespace openjoey::engine::zone
