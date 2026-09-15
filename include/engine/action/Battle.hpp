#pragma once
// ── act/battle — the Battle Step and Damage Step (p.34-39) ──────────────────
#include "engine/action/Chain.hpp"
#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

// Battle Step (p.35): declare an attack; target == nullptr -> direct attack.
// The attack is HELD OPEN (d.turnState.pending): ResolveDamage finishes it,
// ConfirmAttack re-validates (Replay rules, p.37).
ActionResult DeclareAttack(Duel &d, Card *c, Card *target);

// Cancel a held-open attack (player calls the attack off).
void CancelAttack(Duel &d);

// Replay check (p.37): is the held-open attack still valid? If false, the
// attack is cancelled — the monster has NOT used its attack yet.
bool ConfirmAttack(Duel &d);

// Damage Step (p.38), classic math on effective stats. Face-down defenders
// flip face-up first (visibility only); the flipped card lands in
// d.pendingTriggers — whether it has an effect is a spec-provider concern.
ActionResult ResolveDamage(Duel &d);

}  // namespace openjoey::engine::action
