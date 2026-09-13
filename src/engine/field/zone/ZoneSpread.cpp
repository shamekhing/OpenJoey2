#include "engine/field/zone/ZoneSpread.hpp"

namespace openjoey::engine::zone {

ZoneSpread::ZoneSpread(int n, ZoneType zt) : zones_(n, Zone(zt)) { type_ = zt; }

bool ZoneSpread::isEmpty() const {
    for (const auto &zone : zones_)
        if (!zone.isEmpty()) return false;
    return true;
}

void ZoneSpread::reset() { clear(); }

int ZoneSpread::count() const {
    int count = 0;
    for (const auto &zone : zones_)
        if (!zone.isEmpty()) ++count;
    return count;
}

int ZoneSpread::firstEmpty() const {
    for (int index = 0; index < zones_.size(); ++index)
        if (zones_[index].isEmpty()) return index;
    return -1;
}

int ZoneSpread::firstOccupied() const {
    for (int index = 0; index < zones_.size(); ++index)
        if (!zones_[index].isEmpty()) return index;
    return -1;
}

bool ZoneSpread::put(Card *card) {
    if (!card) return false;
    for (auto &zone : zones_)
        if (zone.isEmpty()) return zone.put(card);
    return false;
}

bool ZoneSpread::contains(const Card *card) const {
    for (const auto &zone : zones_)
        if (zone.contains(card)) return true;
    return false;
}

bool ZoneSpread::contains(const std::string &name) const {
    for (const auto &zone : zones_)
        if (zone.contains(name)) return true;
    return false;
}

bool ZoneSpread::replacePtr(Card *from, Card *to) {
    bool replaced = false;
    for (auto &zone : zones_)
        replaced = zone.replacePtr(from, to) || replaced;
    return replaced;
}

Card *ZoneSpread::peek(int index = 0) const {
    if (isEmpty() || index<0 || capacity()<index) 
        return nullptr;
    return zones_[index].peek(); 
}

Card *ZoneSpread::remove(Card *card) {
    Card *out = nullptr;
        for (auto &zone : zones_)
            if(zone.contains(card))
                return zone.remove(card);
    return out;
}

void ZoneSpread::clear() {
    for (auto &zone : zones_) zone.reset();
}

void ZoneSpread::shuffle() { std::shuffle(zones_.begin(), zones_.end(), std::mt19937()); }

std::vector<Card *> ZoneSpread::cards(bool emptyOk) const {
    std::vector<Card *> result;
    for (const auto &zone : zones_)
        if (emptyOk || !zone.isEmpty()) 
            result.push_back(zone.peek());
    return result;
}

}  // namespace openjoey::engine::zone
