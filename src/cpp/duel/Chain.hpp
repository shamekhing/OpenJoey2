#pragma once
#include "card/Card.hpp"
#include "effect/Effect.hpp"
#include <iostream>
#include <memory>
#include <vector>

namespace openjoey::game {

/**
 * One pending effect activation on the chain.
 *
 * The chain owns `effect`; `sourceCard` is a non-owning pointer into DuelCore's
 * card pools.
 */
struct ChainLink {
  std::unique_ptr<Effect> effect;
  int                     player     = -1;
  Card                   *sourceCard = nullptr;
  bool                    negated    = false; // set by NegateActivationEffect
};

/**
 * Activation chain with LIFO resolution and spell-speed gating.
 *
 * Resolution walks from newest link to oldest. The current implementation
 * enforces monotonic spell speed against the top link; richer timing windows
 * can layer on top of canAdd().
 */
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

  // Read access for external inspection.
  const ChainLink &linkAt(int i) const { return links_[i]; }

private:
  std::vector<ChainLink> links_;
  int  resolveIdx_ = -1;
  bool negateNext_ = false;
};

} // namespace openjoey::game
