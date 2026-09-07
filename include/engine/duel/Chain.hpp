#pragma once
#include "action/ActionArgs.hpp" // ActionArgs
#include "action/ActionSpec.hpp"
#include "engine/protocol/BattleProtocol.hpp"
#include "engine/protocol/ChainProtocol.hpp" // ChainStep walk state
#include <cstdint>
#include <vector>

namespace openjoey::engine {
using action::ActionArgs;

// ── Chain (rulebook chaining section) ────────────────────────────────────────
// A stack of card-effect activations. Resolved in *reverse* activation order
// (last activated resolves first). Each link carries its full ActionSpec so
// the resolver needs nothing else at resolution time.
struct Chain {
  struct Link {
    ActionId id;          // convenience mirror of spec.id
    int activator;        // player index that activated it
    uint8_t speed;        // Spell Speed 1 / 2 / 3
    ActionSpec spec;      // the complete action definition
    ActionArgs args;      // runtime arguments used when this link resolves
    bool negated = false; // set when a counter effect negates this activation
  };

  std::vector<Link> links;
  protocol::ChainStep step = protocol::ChainStep::Idle; // resolution walk state
  int consecutivePasses = 0; // p.45 response window: both pass -> resolve

  void push(const ActionSpec &spec, int activator,
            const ActionArgs &args = {}) {
    links.push_back({spec.id, activator, spec.speed, spec, args});
    step = protocol::ChainStep::Building;
    consecutivePasses = 0; // a new activation reopens the response window
  }

  void clear() {
    links.clear();
    step = protocol::ChainStep::Idle;
    consecutivePasses = 0;
  }

  // Resolution order: last link first, down to link 0.
  std::vector<const Link *> resolutionOrder() const {
    std::vector<const Link *> order;
    order.reserve(links.size());
    for (auto it = links.rbegin(); it != links.rend(); ++it)
      order.push_back(&*it);
    return order;
  }

  // ── Spell Speed rule ───────────────────────────────────────────────────────
  // * Spell Speed 1 can never be Chain Link 2 or higher.
  // * A response must have Spell Speed equal to or higher than the link it
  //   responds to (Spell Speed 3 / Counter Traps can respond to anything).
  bool legalToChain(uint8_t speed) const {
    if (links.empty())
      return true; // starting a new chain: any Spell Speed may lead
    return speed > 1 && speed >= links.back().speed;
  }
};

} // namespace openjoey::engine
