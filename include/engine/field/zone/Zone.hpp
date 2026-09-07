#pragma once
// ── Zone (openjoey::zone) ────────────────────────────────────────────────────
// A single-card slot: monster zone, spell/trap zone, field zone, EMZ.
// Split out of the old monolithic zone/Zone.hpp — the class name matches the
// header name, mirroring the openjoey-cards convention.

#include "engine/field/zone/IZone.hpp"

namespace openjoey::engine::zone {
using cards::Card;

class Zone : public IZone {
public:
  Zone() : card_(nullptr) {};
  bool isEmpty() const override { return card_ == nullptr; }
  bool contains(const Card *c) const override { return card_ && card_ == c; }

  bool replacePtr(Card *from, Card *to) override {
    return card_ == from ? (card_ = to, true) : false;
  }

  int count() const override { return card_ ? 1 : 0; }

  void reset() override { card_ = nullptr; }
  Card *peek() const { return card_; }

  bool put(Card *c) override {
    return (card_ || !c) ? false : (card_ = c, true);
  }

  // nullptr removes the occupant; non-null removes only if it matches.
  Card *remove(Card *c = nullptr) override {
    Card *out = card_;
    return isEmpty() || (c && c != card_) ? nullptr : (card_ = nullptr, out);
  }

  // Face-up/face-down state change (set vs activate). Refuses on an empty
  // zone: callers must never set visibility on nothing, and "true" always
  // means the zone is occupied with the requested visibility.
  bool changeVisibility(Visibility v) {
    return isEmpty() ? false : (vis_ = v, true);
  }

protected:
  Card *card_ = nullptr;
};

} // namespace openjoey::engine::zone
