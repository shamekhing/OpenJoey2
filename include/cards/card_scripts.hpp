#ifndef OPENJOEY_CARDS_CARD_SCRIPTS_H_
#define OPENJOEY_CARDS_CARD_SCRIPTS_H_

// ── Card -> action link (cards/card_scripts) ─────────────────────────────────
// Layer 1 (derive): timing/speed/continuous derived from the card's own
//   race/type attributes — covers every card in the database, zero handwork.
// Layer 2 (overlay): data/card_actions.json — the ops a specific card
//   realizes, as an ActionSpec. Starts (near) empty; grows by editing data.
// The engine stays card-blind: callers (UI menus, pendingTriggers loop) ask
// ScriptFor/ResolveScript and pass the returned spec to the engine.

#include <cstdint>
#include <string>
#include <vector>

#include "action/ActionSpec.hpp"
#include "cards/card.hpp"

namespace openjoey::cards {

// Load the overlay file (data/card_actions.json). Safe to call repeatedly;
// the last load wins. Returns the number of scripts loaded (-1 on error).
int LoadCardScripts(const std::string &path);

// Layer 2: the card's wired spec, or nullptr when it is a vanilla card.
const ActionSpec *ScriptFor(uint32_t card_id);

// Layer 1: timing/speed/continuous derived from race + humanReadableCardType.
// `id` stays None — derivation never invents ops.
ActionSpec DeriveScript(const Card &card);

// Layer 1 + Layer 2 merged: overlay ops on top of derived timing/speed.
// id == None means the card realizes nothing (callers gate the menu on it).
ActionSpec ResolveScript(const Card &card);

// Convenience for menu gating.
bool HasScript(uint32_t card_id);

}  // namespace openjoey::cards

#endif  // OPENJOEY_CARDS_CARD_SCRIPTS_H_
