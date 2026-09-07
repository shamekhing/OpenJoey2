#pragma once
// ── ActionSpec (openjoey) ────────────────────────────────────────────────────
// The ONE effect encoding. A card's identity is the set of ActionSpecs it
// carries (openjoey-cards Card.hpp); the gameplay ActionResolver turns each
// spec into concrete zone moves / LP changes. There is deliberately no other
// effect struct: the catalog, the cards and the chain links all speak
// ActionSpec.
#include "action/ActionId.hpp"
#include <cstdint>

namespace openjoey {

// ── Effect timing ────────────────────────────────────────────────────────────
enum class EffectType : uint8_t {
    Ignition,     // activate during your Main Phase        (Spell Speed 1)
    Trigger,      // "when X happens" (includes Flip)
    Quick,        // Spell Speed 2 or higher
    Continuous,   // active while face-up on the field
    Cost,         // paid before activation (Tribute/Discard/Pay-LP...)
};

// ── TargetScope: who/what an effect hits without extra arguments ─────────────
// Replaces the old sentinel protocol (targetPlayer -1/-2/-3/-4) with named,
// checkable values. The resolver reads this off the spec — never off magic
// numbers in ActionArgs.
enum class TargetScope : uint8_t {
    None = 0,       // no implicit targets; runtime args carry everything
    Targeted,       // one explicit card target (ActionArgs::target)
    Activator,      // the activator themself (self-gain, self-discard)
    Opponent,       // the activator's opponent
    PerOppMonster,  // amount × each opponent monster (Just Desserts)
    OppMonsters,    // ALL monsters the opponent controls (Raigeki)
    AllMonsters,    // all monsters on both sides (Dark Hole)
    AllSpellsTraps, // all Spells/Traps on both sides (Heavy Storm, Trunade)
    OppAttackPos,   // opponent's Attack-Position monsters (Mirror Force)
};

// ── ActionSpec ───────────────────────────────────────────────────────────────
// ABI/API CONTRACT: ActionSpec is brace-initialized positionally by external
// repos (e.g. openjoey-gameplay tests:
// `ActionSpec{id, timing, speed, amount, lpCost, scope}`). Do NOT reorder,
// insert, or remove fields without a coordinated version bump (docs/API.md).
struct ActionSpec {
    ActionId    id          = ActionId::None;
    EffectType  timing      = EffectType::Ignition;
    uint8_t     speed       = 1;     // Spell Speed: 1 / 2 / 3
    int         amount      = 1;     // draws / damage / LP gain / cards moved
    int         lpCost      = 0;     // non-refundable activation cost
    TargetScope scope       = TargetScope::None;
    bool        needsTarget = false; // activation must ask for a target card
    const char *note        = "";    // menu label / debug hint
};

} // namespace openjoey
