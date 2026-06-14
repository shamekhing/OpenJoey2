#pragma once
#include "effect/Effect.hpp"

namespace openjoey {

// Forward declaration — Chain is in Layer 4; we access it through IDuelContext.
// IDuelContext exposes negateTopChainLink() for this effect.

/**
 * Counter-speed effect that marks the next resolving chain link as negated.
 */
class NegateActivationEffect : public Effect {
public:
  NegateActivationEffect() { spellSpeed = 3; }

  // Can activate as long as there is at least one other link on the chain.
  // The chain itself enforces that new links must match or exceed the top
  // speed, so the IDuelContext::phase() check is not needed here.
  bool condition(IDuelContext & /*ctx*/) const override {
    // Chain depth check is enforced by Chain::canAdd — always true here
    return true;
  }

  void cost(IDuelContext & /*ctx*/) override {}

  void activate(IDuelContext & /*ctx*/) override {}

  // Called during LIFO resolution. The top-most link (this effect) resolves
  // first and asks DuelCore to remove and discard the next link's source card.
  // DuelCore::resolveChain() handles the actual link removal.
  // We set a flag on the context so DuelCore knows to negate the next link.
  void resolve(IDuelContext &ctx) override {
    ctx.negateNextChainLink(); // DuelCore fulfils this by skipping next link's resolve
  }
};

} // namespace openjoey
