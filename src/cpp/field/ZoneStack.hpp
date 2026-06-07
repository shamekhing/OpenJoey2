#pragma once
#include "IZone.hpp"
#include "Type.hpp"
#include "card/Card.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

namespace openjoey::zone {

// ─── ZoneStack

// Top = back of vector, bottom = front.
class ZoneStack : public IZone {
public:
  ZoneStack() {
    cards_ = {};
    isStack_ = true;
  };
  bool isEmpty() const override { return cards_.empty(); }
  void reset() override { clear(); }

  int count() const override { return static_cast<int>(cards_.size()); }
  // Peek at a card by index (-1 = top/back).
  Card *peek(int index = -1) const override {
    if (cards_.empty())
      return nullptr;
    if (index < 0)
      return cards_.back();
    return index < count() ? cards_[static_cast<std::size_t>(index)] : nullptr;
  }

  // Push to top (back).
  bool put(Card *c) override {
    return !c ? false : (cards_.push_back(c), true);
  }

  bool contains(const Card *c) const override {
    return std::find(cards_.begin(), cards_.end(), c) != cards_.end();
  }

  // nullptr → remove top (back); non-null → remove that specific card.
  Card *remove(Card *c = nullptr) override {
    if (cards_.empty())
      return nullptr;

    if (!c) {
      c = cards_.back();
      cards_.pop_back();
      return c;
    }

    auto it = std::find(cards_.begin(), cards_.end(), c);

    if (it == cards_.end())
      return nullptr;

    cards_.erase(it);
    return c;
  }

  // Find all cards matching predicate. Does not remove them.
  std::vector<Card *> findAll(std::function<bool(const Card *)> pred) const {
    std::vector<Card *> result;
    for (Card *c : cards_)
      if (pred(c))
        result.push_back(c);
    return result;
  }

  void clear() { cards_.clear(); }
  void shuffle() { std::shuffle(cards_.begin(), cards_.end(), std::mt19937()); }

  const std::vector<Card *> &cards() const { return cards_; }

protected:
  std::vector<Card *> cards_ = {};
};

} // namespace openjoey::zone
