#pragma once
#include "card/Card.hpp"
#include <algorithm>
#include <functional>
#include <random>
#include <vector>

#define vcit std::vector<Card *>::iterator
#define vcfi(c, vc) std::find(vc.begin(), vc.end(), c)

namespace openjoey::zone {

// ─── ZoneType
enum class ZoneType : uint8_t {
  Monster,
  SpellTrap,
  Field,
  ExtraMonster,
  Hand,
  Deck,
  ExtraDeck,
  Graveyard,
  Banished,
  SideDeck,
};

enum class Orientation : uint8_t {
  Vertical,
  Horizontal,
};

enum class Visibility : uint8_t {
  Visible,
  Limited,
  Restricted,
};

// ─── IZone
// base for all zone types. A zone owns no cards — it holds raw pointers.
// Ownership is managed by the game engine.
class IZone {
public:
  virtual ~IZone() = default;

  virtual ZoneType type() const = 0;
  virtual bool isEmpty() const = 0;
  virtual int count() const = 0;

  virtual bool put(Card *c) = 0;
  virtual bool contains(const Card *c) const = 0;
  virtual void reset() {}

  // nullptr → remove top/only occupant.
  virtual Card *remove(Card *c = nullptr) = 0;

  // Move the top/only card to dest. Rolls back if dest.put fails.
  bool moveTo(IZone &dest) {
    Card *c = remove(nullptr);
    return !c ? false : (!dest.put(c) ? (put(c), false) : true);
  }

  bool isVertical() const { return ori_ == Orientation::Vertical; }
  bool isHorizontal() const { return ori_ == Orientation::Horizontal; }
  bool isVisible() const { return vis_ == Visibility::Visible; }
  bool isLimited() const { return vis_ == Visibility::Limited; }
  bool isRestricted() const { return vis_ == Visibility::Restricted; }

protected:
  Orientation ori_ = Orientation::Vertical;
  Visibility vis_ = Visibility::Visible;
};

// ─── Zone
// ───────────────────────────────────────────────────────────────────── A
// single-card slot: monster zone, spell/trap zone, field zone, EMZ.
class Zone : public IZone {
public:
  Zone() : card_(nullptr) {};
  bool isEmpty() const override { return card_ == nullptr; }
  bool contains(const Card *c) const override { return card_ && card_ == c; }

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

protected:
  Card *card_ = nullptr;
};

// ─── ZoneStack

// Top = back of vector, bottom = front.
class ZoneStack : public IZone {
public:
  ZoneStack() { cards_ = {}; };
  bool isEmpty() const override { return cards_.empty(); }
  void reset() override { clear(); }

  int count() const override { return static_cast<int>(cards_.size()); }
  // Peek at a card by index (-1 = top/back).
  Card *peek(int index = -1) const {
    return cards_.empty() || count() <= index ? nullptr
           : index < 0                        ? cards_.back()
                                              : cards_[index];
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

// ─── Concrete zone types

// Main Monster Zone — 5 per player.
// Tracks ATK/DEF orientation and face-up/face-down state.
class Zone_Monster : public Zone {
public:
  ZoneType type() const override { return ZoneType::Monster; }
  Orientation position() const { return ori_; }
  Visibility visibility() const { return vis_; }

  bool changeOrientation(Orientation p) {
    return isEmpty() || (!isVisible() && p == ori_) ? false : (ori_ = p, true);
  }

  bool changeVisibility(Visibility v) {
    return isEmpty() && v != vis_ ? false : (vis_ = v, true);
  }
  // Flip summon: face-down defense → face-up attack or defense.
  bool flip() {
    if (ori_ == Orientation::Vertical && vis_ == Visibility::Limited) {
      ori_ = Orientation::Horizontal;
      vis_ = Visibility::Visible;
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
  ZoneType type() const override { return ZoneType::SpellTrap; }

private:
};

// Field Spell Zone — 1 per player.
// Field spells activate immediately when placed; destroying one sends to GY.
class Zone_Field : public Zone {
public:
  ZoneType type() const override { return ZoneType::Field; }
};

// Extra Monster Zone — 2 on the mat, shared between players.
// Used for Link Monsters and co-linked Xyz summoned to the EMZ.
class Zone_ExtraMonster : public Zone_Monster {
public:
  ZoneType type() const override { return ZoneType::ExtraMonster; }
};

// Hand Zone — cards held by the player; hidden from the opponent.
class ZoneStack_Hand : public ZoneStack {
public:
  ZoneType type() const override { return ZoneType::Hand; }
};

// Deck Zone — player's main deck. Top = back.
class ZoneStack_Deck : public ZoneStack {
public:
  ZoneType type() const override { return ZoneType::Deck; }

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
  ZoneType type() const override { return ZoneType::ExtraDeck; }
};

// Graveyard — cards sent here are face-up and visible to both players.
class ZoneStack_Graveyard : public ZoneStack {
public:
  ZoneType type() const override { return ZoneType::Graveyard; }
};

// Banished Zone — cards can be banished face-up or face-down.
// Face-down banished cards are not revealed and generally cannot be interacted
// with (Rulebook p.49).  Such cards live in faceDownCards_ and are tracked by
// the overrides below so they still count / can be found / removed.
class ZoneStack_Banished : public ZoneStack {
public:
  ZoneType type() const override { return ZoneType::Banished; }

  bool isEmpty() const override {
    return cards_.empty() && faceDownCards_.empty();
  }
  int count() const override {
    return static_cast<int>(cards_.size() + faceDownCards_.size());
  }
    bool contains(const Card *c) const override {
    return ZoneStack::contains(c) ||
           std::find(faceDownCards_.begin(), faceDownCards_.end(), c) !=
               faceDownCards_.end();
  }

  // nullptr -> remove top visible card; c -> remove that specific card
  // (searches both face-up and face-down piles).
  Card *remove(Card *c = nullptr) override {
    Card *r = ZoneStack::remove(c);
    if (r)
      return r;
    if (!c) {
      if (faceDownCards_.empty())
        return nullptr;
      Card *top = faceDownCards_.back();
      faceDownCards_.pop_back();
      return top;
    }
    auto it = std::find(faceDownCards_.begin(), faceDownCards_.end(), c);
    if (it == faceDownCards_.end())
      return nullptr;
    Card *out = *it;
    faceDownCards_.erase(it);
    return out;
  }

  // Face-down banish (e.g. cost that removes a card hidden).
  Card *putFaceDown(Card *c) {
    if (!c)
      return nullptr;
    faceDownCards_.push_back(c);
    return c;
  }

private:
  std::vector<Card *> faceDownCards_;
};

// Side Deck — used between duels to swap cards in/out of the main/extra deck.
class ZoneStack_SideDeck : public ZoneStack {
public:
  ZoneType type() const override { return ZoneType::SideDeck; }
};

} // namespace openjoey::zone
