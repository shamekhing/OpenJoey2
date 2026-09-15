#pragma once
// act/Action - THE interpreter (the one dispatcher of the layer).
// `apply()` realizes one `Action{op, scope, amount}` against the mat through
// the Move.hpp primitives. Every path that runs an op - chain-link resolution,
// cost lists, (later) card specs - goes through here. There is no second
// dispatcher: Perform is gone, the chain resolver calls apply().
// Scope resolution (which cards an Action hits) also lives here, so
// "Op x Scope" semantics are readable in exactly one place.
#include <vector>

#include "action/ActionArgs.hpp"
#include "action/ActionResult.hpp"
#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::Action;
using openjoey::ActionId;
using openjoey::Scope;
using cards::Card;

// The execution context of one realized Action.
struct ActionCtx {
    Duel &d;
    int activator;      // player realizing the action
    ActionArgs args;    // target / source / n / materials / targetPlayer override
};

// Scope -> concrete card list (the single home of scope semantics).
std::vector<Card *> resolveScope(const Action &a, const ActionCtx &c);

// THE dispatcher: one Action -> mat primitives. Never a silent success.
ActionResult apply(const Action &a, ActionCtx &c);

}  // namespace openjoey::engine::action
