#pragma once
#include "card/Card.hpp"
#include "effect/Effect.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace openjoey::game {

// ─── ChainLink ────────────────────────────────────────────────────────────────

struct ChainLink {
  std::unique_ptr<Effect> effect;
  int                     player     = -1;
  Card                   *sourceCard = nullptr;
  bool                    negated    = false; // set by NegateActivationEffect
};

// ─── Chain ────────────────────────────────────────────────────────────────────
// Activation chain with LIFO resolution and YGO spell-speed rules.
//
// Spell-speed rules enforced in canAdd():
//   • Each new link must have spellSpeed >= the current top link's spellSpeed.
//   • A speed-3 link (counter-trap) can only respond to speed >= 1.
//     (Chain::canAdd enforces: new spellSpeed >= top spellSpeed.)
//   • Speed-1 effects cannot be added to a non-empty chain.
//
// Resolution: resolveNext() walks from back (most recent) to front (oldest).

class Chain {
public:
  // Returns false if spell-speed rules are violated or effect is null.
  bool canAdd(const Effect *effect) const {
    if (!effect) return false;
    if (links_.empty()) return true;
    return effect->spellSpeed >= topSpellSpeed();
  }

  // Push a new link onto the chain. Takes ownership of the Effect.
  // Returns false (and logs) if canAdd() fails.
  bool push(std::unique_ptr<Effect> effect, int player, Card *sourceCard = nullptr) {
    if (!canAdd(effect.get())) {
      std::cerr << "[Chain] push rejected: spellSpeed "
                << (effect ? effect->spellSpeed : -1)
                << " < top " << topSpellSpeed() << "\n";
      return false;
    }
    ChainLink link;
    link.effect     = std::move(effect);
    link.player     = player;
    link.sourceCard = sourceCard;
    links_.push_back(std::move(link));
    resolveIdx_ = static_cast<int>(links_.size()) - 1;
    return true;
  }

  // Resolve the next unresolved link (LIFO — from back to front).
  // Returns nullptr when all links are resolved.
  // Caller (DuelCore) must check hasPendingTarget() before calling.
  ChainLink *resolveNext(openjoey::IDuelContext &ctx) {
    if (resolveIdx_ < 0) return nullptr;
    ChainLink &link = links_[resolveIdx_];
    --resolveIdx_;

    if (link.negated) {
      // Send negated source card to GY
      if (link.sourceCard) {
        link.sourceCard->location = etypes::location::Graveyard;
        ctx.field().graveyardZones[link.sourceCard->owner].put(link.sourceCard);
      }
      return &link;
    }

    if (link.effect)
      link.effect->resolve(ctx);

    // After a NegateActivationEffect resolves, it called ctx.negateNextChainLink().
    // DuelCore sets negateNextLink_ which we honour on the next call.
    if (negateNext_ && resolveIdx_ >= 0) {
      links_[resolveIdx_].negated = true;
      negateNext_ = false;
    }

    return &link;
  }

  // Resolve every remaining link in LIFO order.
  void resolveAll(openjoey::IDuelContext &ctx) {
    while (resolveIdx_ >= 0)
      resolveNext(ctx);
    links_.clear();
    resolveIdx_ = -1;
    negateNext_ = false;
  }

  // Called by DuelCore when IDuelContext::negateNextChainLink() is invoked.
  void requestNegateNext() { negateNext_ = true; }

  bool isEmpty()   const { return links_.empty(); }
  int  size()      const { return static_cast<int>(links_.size()); }
  int  topSpellSpeed() const {
    return links_.empty() ? 0 : links_.back().effect->spellSpeed;
  }

  void clear() {
    links_.clear();
    resolveIdx_ = -1;
    negateNext_ = false;
  }

  // Read access for UI inspection.
  const ChainLink &linkAt(int i) const { return links_[i]; }

private:
  std::vector<ChainLink> links_;
  int  resolveIdx_ = -1;
  bool negateNext_ = false;
};

} // namespace openjoey::game
