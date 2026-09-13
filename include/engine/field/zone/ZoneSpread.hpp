#pragma once
// ── ZoneSpread (openjoey::zone) ───────────────────────────────────────────────
// A multi-card zone: hand, deck, extra deck, graveyard, banished, side deck.
// Top = back of vector, bottom = front.
// Split out of the old monolithic zone/Zone.hpp — the class name matches the
// header name, mirroring the openjoey-cards convention.

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#include "engine/field/zone/IZone.hpp"
#include "engine/field/zone/Zone.hpp"

namespace openjoey::engine::zone {
using cards::Card;

class ZoneSpread : public IZone {
   public:
    explicit ZoneSpread(int n = 0, ZoneType zt = ZoneType::None);
    bool isEmpty() const override;
    void reset() override;
    int count() const override;

    int capacity() const { return static_cast<int>(zones_.size()); }
    int firstEmpty() const;
    int firstOccupied() const;
    Zone &operator[](int index) { return zones_.at(index); }
    const Zone &operator[](int index) const { return zones_.at(index); }
    auto begin() { return zones_.begin(); }
    auto end() { return zones_.end(); }
    auto begin() const { return zones_.begin(); }
    auto end() const { return zones_.end(); }

    // Push to first empty zone 
    bool put(Card *c) override;
    bool contains(const Card *c) const override;
    bool contains(const std::string &name) const override;
    bool replacePtr(Card *from, Card *to) override;

    // Peek at a card by index (-1 = top/back).
    Card *peek(int index = -1) const override;

    // nullptr → remove top (back); non-null → remove that specific card.
    Card *remove(Card *c = nullptr) override;

    void clear();
    
    void shuffle();

    std::vector<Card *> cards(bool emptyOk = true) const;

   protected:
    std::vector<Zone> zones_ = {};
};

}  // namespace openjoey::engine::zone