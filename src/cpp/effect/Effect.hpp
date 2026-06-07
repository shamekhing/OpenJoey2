#pragma once
#include "card/Card.hpp"
#include "effect/IDuelContext.hpp"

namespace openjoey {

// ─── Effect ───────────────────────────────────────────────────────────────────
// Abstract base for every game effect.
//
// Lifecycle per activation:
//   condition(ctx) — may this effect be offered right now?
//   cost(ctx)      — pay costs (LP, tributes, discards); called only if condition passes
//   activate(ctx)  — place on chain; may push a TargetRequest
//   resolve(ctx)   — called by Chain::resolveNext() in LIFO order
//
// DuelCore::activateEffect() runs condition → cost → chain.push(this) → activate.
// Chain::resolveAll() runs resolve() for each link in reverse push order.

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
