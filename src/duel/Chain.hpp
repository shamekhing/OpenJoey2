#pragma once
#include "card/EffectID.hpp"
#include <cstdint>
#include <vector>

namespace openjoey {

// ── Chain (Rulebook p.41) ───────────────────────────────────────────────────
// A stack of card-effect activations.  Resolved in *reverse* activation order
// (last activated resolves first).  Each link carries the Spell Speed so the
// engine can reject illegal responses.  Layer 4: owns no zones, only the order
// in which field/effect operations fire.
struct Chain {
  struct Link {
    EffectID id;
    int activator;    // player index that activated it
    uint8_t speed;    // Spell Speed 1 / 2 / 3
  };

  std::vector<Link> links;

  void push(EffectID id, int activator, uint8_t speed = 1) {
    links.push_back({id, activator, speed});
  }

  // Resolution order: last link first, down to link 0.
  std::vector<const Link *> resolutionOrder() const {
    std::vector<const Link *> order;
    order.reserve(links.size());
    for (auto it = links.rbegin(); it != links.rend(); ++it)
      order.push_back(&*it);
    return order;
  }

  void clear() { links.clear(); }
};

} // namespace openjoey
