#pragma once
// ── ActionSpec (openjoey) ────────────────────────────────────────────────────
// The ONE action encoding. A card's identity is the set of ActionSpecs it
// carries; the gameplay resolver turns each spec's `actions` list into
// concrete zone moves / LP changes through the interpreter
// (engine/action/Action.hpp). There is deliberately no other effect struct:
// the cards and the chain links all speak ActionSpec.
//
// ABI NOTE: the 47-id rebuild intentionally breaks the old positional
// brace-initialization contract (`ActionSpec{id, timing, speed, amount,
// lpCost, scope}`) — specs are now written with designated initializers.
#include <cstdint>
#include <vector>

#include "action/ActionId.hpp"

namespace openjoey {

// ── Action timing ────────────────────────────────────────────────────────────
enum class ActionType : uint8_t {
    Ignition,    // activate during your Main Phase        (Spell Speed 1)
    Trigger,     // "when X happens" (includes Flip)
    Quick,       // Spell Speed 2 or higher
    Continuous,  // active while face-up on the field
    Cost,        // paid before activation (Tribute/Discard/Pay-LP...)
};

// ── ActionSpec ───────────────────────────────────────────────────────────────
struct ActionSpec {
    ActionId id = ActionId::None;
    ActionType timing = ActionType::Ignition;
    uint8_t speed = 1;  // Spell Speed: 1 / 2 / 3
    int lpCost = 0;     // non-refundable activation cost
    bool needsTarget = false;        // activation must ask for a target card
    std::vector<Action> actions;     // the realization: §1–8 ops, resolved in order
    const char *note = "";           // menu label / debug hint
};

}  // namespace openjoey

