#pragma once
// ── act/chains — card-effect activation + chain resolution ──────────────────
// Activation: guards + cost + push. Resolution: last link first; each link's
// spec.actions are realized through the ONE interpreter (Action.hpp::apply).
// Negation blanks the nearest earlier non-negated link. No second dispatcher.
#include <string>
#include <vector>

#include "engine/action/Action.hpp"
#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

// Activate a card effect: Spell Speed legality enforced; spec.lpCost charged
// here once (never refunded, even if later negated); link pushed onto chain.
ActionResult ActivateEffect(Duel &d, const ActionSpec &spec, int activator, const ActionArgs &args = {});

// Resolve the chain: last link first. Each link's spec.actions are realized
// through apply() — the same interpreter the costs use (no second dispatcher).
ActionResult ResolveChainImpl(Duel &d, int depth);

// Pass the response window (p.45). With d.config.chainResponseWindow enabled,
// a chain resolves only once BOTH players pass consecutively; any new
// activation (ActivateEffect) reopens the window. With the flag off (default),
// chains resolve explicitly and passing is a no-op.
inline ActionResult ResolveChain(Duel &d);  // fwd: PassResponse resolves on both-pass

ActionResult PassResponse(Duel &d, int player);

bool ChainWaiting(const Duel &d);

ActionResult ResolveChain(Duel &d);

}  // namespace openjoey::engine::action
