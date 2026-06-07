#pragma once
#include "effect/Effect.hpp"
#include "field/Zone.hpp"

namespace openjoey {

// ─── ContinuousEffect ────────────────────────────────────────────────────────
// Base for effects that persist on the field (Continuous spells/traps,
// field spells, equip spells). Subclass and override applyTick() for the
// per-tick passive behaviour.
//
// On activate(), registers itself with ZoneEffectManager via IDuelContext.
// ZoneEffectManager calls applyTick() each tick while the source card
// remains in the source zone; auto-removes the entry when it leaves.

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
