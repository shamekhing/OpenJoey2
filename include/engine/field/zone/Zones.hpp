#pragma once
// ── Concrete zone types (openjoey::zone) ─────────────────────────────────────
// One header for all Zone/ZoneStack flavours. Split out of the old monolithic
// zone/Zone.hpp; the two macros that lived with them (vcit / vcfi) were dead
// code and are gone.

#include "engine/field/zone/Zone.hpp"
#include "engine/field/zone/ZoneStack.hpp"

namespace openjoey::engine::zone {
using cards::Card;

// ─── Single-card slots ──────────────────────────────────────────────────────

// Main Monster Zone — 5 per player.
// Tracks ATK/DEF orientation and face-up/face-down state.
class Zone_Monster : public Zone {
   public:
    ZoneType type() const override { return ZoneType::Monster; }
    Orientation position() const { return ori_; }

    bool changeOrientation(Orientation p) { return isEmpty() || (!isVisible() && p == ori_) ? false : (ori_ = p, true); }

    // Flip Summon (Rulebook): face-down Defense → face-up Attack.
    // Canonical set state = Horizontal + Limited (controller knows the card,
    // opponent doesn't). After the flip both players see it (Vertical + Visible).
    bool flip() {
        if (ori_ == Orientation::Horizontal && vis_ == Visibility::Limited) {
            ori_ = Orientation::Vertical;
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

// ─── Stacks ─────────────────────────────────────────────────────────────────

// Hand Zone — cards held by the player. Limited: exactly one player (the
// owner) knows these cards; the opponent does not.
class ZoneStack_Hand : public ZoneStack {
   public:
    ZoneStack_Hand() { vis_ = Visibility::Limited; }
    ZoneType type() const override { return ZoneType::Hand; }
};

// Deck Zone — player's main deck. Top = back. Restricted: neither player
// knows the deck's order (until a card is drawn/revealed).
class ZoneStack_Deck : public ZoneStack {
   public:
    ZoneStack_Deck() { vis_ = Visibility::Restricted; }
    ZoneType type() const override { return ZoneType::Deck; }

    // Draw one card from the top.
    bool draw(IZone &hand) { return moveTo(hand); }

    // Mill n cards from the top to dest (e.g., graveyard). Returns count sent.
    bool mill(int n, IZone &dest) {
        for (int i = 0; i < n && !isEmpty(); ++i) {
            if (!draw(dest)) return false;
        }
        return true;
    }
};

// Extra Deck — Fusion/Synchro/Xyz/Link/Pendulum monsters. Face-down at rest:
// Limited (the owner knows what they built; the opponent does not).
class ZoneStack_ExtraDeck : public ZoneStack {
   public:
    ZoneStack_ExtraDeck() { vis_ = Visibility::Limited; }
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

    bool isEmpty() const override { return cards_.empty() && faceDownCards_.empty(); }
    int count() const override { return static_cast<int>(cards_.size() + faceDownCards_.size()); }
    bool contains(const Card *c) const override { return ZoneStack::contains(c) || std::find(faceDownCards_.begin(), faceDownCards_.end(), c) != faceDownCards_.end(); }

    // nullptr -> remove top visible card; c -> remove that specific card
    // (searches both face-up and face-down piles).
    Card *remove(Card *c = nullptr) override {
        Card *r = ZoneStack::remove(c);
        if (r) return r;
        if (!c) {
            if (faceDownCards_.empty()) return nullptr;
            Card *top = faceDownCards_.back();
            faceDownCards_.pop_back();
            return top;
        }
        auto it = std::find(faceDownCards_.begin(), faceDownCards_.end(), c);
        if (it == faceDownCards_.end()) return nullptr;
        Card *out = *it;
        faceDownCards_.erase(it);
        return out;
    }

    // Face-down banish (e.g. cost that removes a card hidden).
    Card *putFaceDown(Card *c) {
        if (!c) return nullptr;
        faceDownCards_.push_back(c);
        return c;
    }

   private:
    std::vector<Card *> faceDownCards_;
};

// Side Deck — used between duels to swap cards in/out of the main/extra deck.
// Limited: only its owner knows its contents.
class ZoneStack_SideDeck : public ZoneStack {
   public:
    ZoneStack_SideDeck() { vis_ = Visibility::Limited; }
    ZoneType type() const override { return ZoneType::SideDeck; }
};

}  // namespace openjoey::engine::zone