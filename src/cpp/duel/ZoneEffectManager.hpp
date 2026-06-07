#pragma once
#include "effect/Effect.hpp"
#include "effect/IDuelContext.hpp"
#include "field/Zone.hpp"
#include <algorithm>
#include <vector>

namespace openjoey::game {

// ─── ZoneEffectManager ────────────────────────────────────────────────────────
// Tracks persistent effects (Continuous, Equip, Field) that stay active as
// long as their source card remains in its source zone.
//
// No callbacks into Zone.hpp are needed — we poll source zone occupancy on
// each tick(). Entries whose source card has left the zone are removed and
// the effect's cleanup() is called (if overridden by the subclass).

class ZoneEffectManager {
public:
  struct Entry {
    openjoey::Effect         *effect    = nullptr;
    openjoey::zone::IZone    *srcZone   = nullptr;
    openjoey::Card           *srcCard   = nullptr;
  };

  // Called by ContinuousEffect::activate() via IDuelContext.
  void registerEffect(openjoey::Effect        *eff,
                      openjoey::zone::IZone   *srcZone,
                      openjoey::Card          *srcCard) {
    entries_.push_back({eff, srcZone, srcCard});
  }

  // Called every game tick (phase boundary, after chain resolve, etc.).
  // Applies all active persistent effects and removes stale ones.
  void tick(openjoey::IDuelContext &ctx) {
    entries_.erase(
        std::remove_if(entries_.begin(), entries_.end(),
            [&](Entry &e) -> bool {
              if (!e.srcZone || !e.srcCard) return true;
              if (!e.srcZone->contains(e.srcCard)) {
                // Source left the zone — deactivate.
                return true;
              }
              // Apply passive effect (calls Effect::applyTick, overridden by ContinuousEffect).
              if (e.effect)
                e.effect->applyTick(ctx);
              return false;
            }),
        entries_.end());
  }

  void clear() { entries_.clear(); }
  int  count() const { return static_cast<int>(entries_.size()); }

private:
  std::vector<Entry> entries_;
};

} // namespace openjoey::game
