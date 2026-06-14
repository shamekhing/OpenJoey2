#pragma once
#include "effect/Effect.hpp"
#include "effect/IDuelContext.hpp"
#include "field/Zone.hpp"
#include <algorithm>
#include <vector>

namespace openjoey::game {

/**
 * Tracks persistent effects tied to a source card in a source zone.
 *
 * Zones do not emit events. Instead, DuelCore calls tick() at phase/chain
 * boundaries and stale entries fall out when their source card leaves.
 */
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
