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
    ZoneStack() { cards_ = {}; };
    bool isEmpty() const override { return cards_.empty(); }
    void reset() override { clear(); }

    int count() const override { return static_cast<int>(cards_.size()); }
    // Peek at a card by index (-1 = top/back).
    Card *peek(int index = -1) const { return cards_.empty() || count() <= index ? nullptr : index < 0 ? cards_.back() : cards_[index]; }

    // Push to top (back).
    bool put(Card *c) override { return !c ? false : (cards_.push_back(c), true); }

    bool contains(const Card *c) const override { return std::find(cards_.begin(), cards_.end(), c) != cards_.end(); }

    // nullptr → remove top (back); non-null → remove that specific card.
    Card *remove(Card *c = nullptr) override {
        if (cards_.empty()) return nullptr;

        if (!c) {
            c = cards_.back();
            cards_.pop_back();
            return c;
        }

        auto it = std::find(cards_.begin(), cards_.end(), c);

        if (it == cards_.end()) return nullptr;

        cards_.erase(it);
        return c;
    }

    // Find all cards matching predicate. Does not remove them.
    std::vector<Card *> findAll(std::function<bool(const Card *)> pred) const {
        std::vector<Card *> result;
        for (Card *c : cards_)
            if (pred(c)) result.push_back(c);
        return result;
    }

    void clear() { cards_.clear(); }
    void shuffle() { std::shuffle(cards_.begin(), cards_.end(), std::mt19937()); }

    bool replacePtr(Card *from, Card *to) override {
        bool hit = false;
        for (Card *&c : cards_)
            if (c == from) {
                c = to;
                hit = true;
            }
        return hit;
    }

    const std::vector<Card *> &cards() const { return cards_; }

   protected:
    std::vector<Card *> cards_ = {};
};

}  // namespace openjoey::engine::zone