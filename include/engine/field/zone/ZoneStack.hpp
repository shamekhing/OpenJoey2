#pragma once
// ── ZoneStack (openjoey::zone) ───────────────────────────────────────────────
// A multi-card zone: hand, deck, extra deck, graveyard, banished, side deck.
// Top = back of vector, bottom = front.
// Split out of the old monolithic zone/Zone.hpp — the class name matches the
// header name, mirroring the openjoey-cards convention.

#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#include "engine/field/zone/IZone.hpp"

namespace openjoey::engine::zone {
using cards::Card;

class ZoneStack : public IZone {
   public:
    explicit ZoneStack(ZoneType zt = ZoneType::None);
    Card *operator[] (size_t i) const override;

    bool isEmpty() const override;
    void reset() override;
    int count() const override;
    bool put(Card *c) override;
    bool contains(const Card *c) const override;
    bool contains(const std::string &name) const override;

    // Peek at a card by index (-1 = top/back).
    Card *peek(int index = -1) const override;
    // nullptr → remove top (back); non-null → remove that specific card.
    Card *remove(Card *c = nullptr) override;

    // Find all cards matching predicate. Does not remove them.
    std::vector<Card *> findAll(std::function<bool(const Card *)> pred) const;

    void shuffle();
    bool replacePtr(Card *from, Card *to) override;

    const std::vector<Card *> &cards() const;

   protected:
    std::vector<Card *> cards_ = {};
};

}  // namespace openjoey::engine::zone