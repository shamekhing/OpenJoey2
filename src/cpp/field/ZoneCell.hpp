#pragma once
#include "IZone.hpp"
#include "Type.hpp"
#include "card/Card.hpp"
#include "field/ZoneStack.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

namespace openjoey::zone {

/**
 * Single-card zone slot: monster zone, spell/trap zone, field zone, or EMZ.
 */
class ZoneCell : public IZone {
public:
  ZoneCell() : card_(nullptr) { isStack_ = false; };
  bool isEmpty() const override { return card_ == nullptr; }
  bool contains(const Card *c) const override { return card_ && card_ == c; }

  int count() const override { return card_ ? 1 : 0; }

  void reset() override { card_ = nullptr; }
  Card *peek(int = -1) const override { return card_; }

  bool put(Card *c) override {
    return (card_ || !c) ? false : (card_ = c, true);
  }

  // nullptr removes the occupant; non-null removes only if it matches.
  Card *remove(Card *c = nullptr) override {
    Card *out = card_;
    return isEmpty() || (c && c != card_) ? nullptr : (card_ = nullptr, out);
  }

protected:
  Card *card_ = nullptr;
};

using Zone = ZoneCell;

// ─── Concrete zone types

// Main Monster Zone — 5 per player.
// Tracks ATK/DEF orientation and face-up/face-down state.
class Zone_Monster : public ZoneCell {
public:
  etypes::zone type() const override { return etypes::zone::Monster; }
  etypes::orientation orientation() const { return orientation_; }
  etypes::visibility visibility() const { return visibility_; }

  bool changeOrientation(etypes::orientation p) {
    return isEmpty() || (!is(etypes::visibility::Visible) && p == orientation_)
               ? false
               : (orientation_ = p, true);
  }

  bool changeVisibility(etypes::visibility v) {
    return isEmpty() && v != visibility_ ? false : (visibility_ = v, true);
  }
  // Flip summon: face-down defense -> face-up attack or defense.
  bool flip() {
    if (orientation_ == etypes::orientation::Vertical &&
        visibility_ == etypes::visibility::Limited) {
      orientation_ = etypes::orientation::Horizontal;
      visibility_ = etypes::visibility::Visible;
      return true;
    }
    return false;
  }

private:
};

// Spell/Trap Zone — 5 per player. Slots 0 and 4 double as Pendulum Zones.
// Cards are placed face-down ("set") and then activated.
class Zone_SpellTrap : public Zone {
public:
  etypes::zone type() const override { return etypes::zone::SpellTrap; }

private:
};

// Field Spell Zone — 1 per player.
// Field spells activate immediately when placed; destroying one sends to GY.
class Zone_Field : public Zone {
public:
  etypes::zone type() const override { return etypes::zone::Field; }
};

// Extra Monster Zone — 2 on the mat, shared between players.
// Used for Link Monsters and co-linked Xyz summoned to the EMZ.
class Zone_ExtraMonster : public Zone_Monster {
public:
  etypes::zone type() const override { return etypes::zone::ExtraMonster; }
};

// Hand Zone — cards held by the player; hidden from the opponent.
class ZoneStack_Hand : public ZoneStack {
public:
  ZoneStack_Hand() { visibility_ = etypes::visibility::Limited; }
  etypes::zone type() const override { return etypes::zone::Hand; }
};

// Deck Zone — player's main deck. Top = back.
class ZoneStack_Deck : public ZoneStack {
public:
  ZoneStack_Deck() { visibility_ = etypes::visibility::Restricted; }
  etypes::zone type() const override { return etypes::zone::Deck; }

  // Draw one card from the top.
  bool draw(IZone &hand) { return moveTo(hand); }

  // Mill n cards from the top to dest (e.g., graveyard). Returns count sent.
  bool mill(int n, IZone &dest) {
    for (int i = 0; i < n && !isEmpty(); ++i) {
      if (!draw(dest))
        return false;
    }
    return true;
  }
};

// Extra Deck — Fusion/Synchro/Xyz/Link/Pendulum monsters. Face-down at rest.
class ZoneStack_ExtraDeck : public ZoneStack {
public:
  etypes::zone type() const override { return etypes::zone::ExtraDeck; }
};

// Graveyard — cards sent here are face-up and visible to both players.
class ZoneStack_Graveyard : public ZoneStack {
public:
  etypes::zone type() const override { return etypes::zone::Graveyard; }
};

// Banished Zone — cards can be banished face-up or face-down.
// Face-down banished cards are not revealed and generally cannot be interacted
// with.
class ZoneStack_Banished : public ZoneStack {
public:
  etypes::zone type() const override { return etypes::zone::Banished; }

private:
  std::vector<Card *> faceDownCards_;
};

// Side Deck — used between duels to swap cards in/out of the main/extra deck.
class ZoneStack_SideDeck : public ZoneStack {
public:
  ZoneStack_SideDeck() { visibility_ = etypes::visibility::Limited; }
  etypes::zone type() const override { return etypes::zone::SideDeck; }
};

} // namespace openjoey::zone
