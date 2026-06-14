#pragma once
#include "card/Card.hpp"
#include "effect/IDuelContext.hpp"

namespace openjoey {

/**
 * Abstract base for every game effect.
 *
 * Activation lifecycle:
 * condition(ctx) -> cost(ctx) -> chain push -> activate(ctx) -> resolve(ctx).
 * `activate` may request targets; `resolve` is called later by Chain in LIFO
 * order.
 */
class Effect {
public:
  virtual ~Effect() = default;

  // 1 = Normal/Ignition, 2 = Quick/Trap, 3 = Counter-trap
  int spellSpeed = 1;

  // Set by DuelCore::activateEffect() before any lifecycle call.
  Card *sourceCard = nullptr;

  // ── Lifecycle ──────────────────────────────────────────────────────────
  virtual bool condition(IDuelContext &ctx) const = 0;
  virtual void cost(IDuelContext &ctx)             = 0;
  virtual void activate(IDuelContext &ctx)         = 0;
  virtual void resolve(IDuelContext &ctx)          = 0;

  // Called by ZoneEffectManager each tick for persistent effects.
  // Default is a no-op; ContinuousEffect subclasses override this.
  virtual void applyTick(IDuelContext & /*ctx*/) {}
};

} // namespace openjoey
