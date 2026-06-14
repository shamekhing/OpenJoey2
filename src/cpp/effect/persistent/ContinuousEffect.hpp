#pragma once
#include "effect/Effect.hpp"
#include "field/Zone.hpp"

namespace openjoey {

/**
 * Base for field-persistent effects.
 *
 * Subclasses override applyTick(). ZoneEffectManager keeps calling it while the
 * source card remains in sourceZone.
 */
class ContinuousEffect : public Effect {
public:
  ContinuousEffect() { spellSpeed = 1; }

  // Source zone is set by the caller (DuelCore) before activateEffect().
  zone::IZone *sourceZone = nullptr;

  bool condition(IDuelContext & /*ctx*/) const override { return true; }
  void cost(IDuelContext & /*ctx*/) override {}

  void activate(IDuelContext &ctx) override {
    if (sourceZone && sourceCard)
      ctx.registerPersistentEffect(this, sourceZone, sourceCard);
  }

  // resolve() is a no-op for continuous effects — their work is done in
  // applyTick(), called by ZoneEffectManager every game tick.
  void resolve(IDuelContext & /*ctx*/) override {}

  // Override in subclasses to apply the passive effect each tick.
  void applyTick(IDuelContext & /*ctx*/) override {}
};

} // namespace openjoey
