#ifndef OPENJOEY_CARDS_CARD_API_H_
#define OPENJOEY_CARDS_CARD_API_H_

// ── cards/card_api — the ONE public door of the cards layer ──────────────────
// Database loading, the classic subset, and the card -> action link.
// Everything in the cards layer is const data: nothing here mutates a Card.

#include <string>

#include "cards/card.hpp"
#include "cards/card_database.hpp"
#include "action/ActionSpec.hpp"

namespace openjoey::cards {

// ── Database ─────────────────────────────────────────────────────────────────
// Load a card database from a YGOProDeck-format JSON file
// ({"data": [...]}). Returns false on parse failure (db is left empty).
bool LoadDatabase(CardDatabase &db, const std::string &path);

// ── Card -> action link ──────────────────────────────────────────────────────
// Layer 1: timing/speed derived from race/type (every card).
// Layer 2: data/card_actions.json overlay (specific ops per card id).
// LoadCardScripts must be called once before ScriptFor/ResolveScript.
int LoadCardScripts(const std::string &path);
const ActionSpec *ScriptFor(uint32_t card_id);   // nullptr = vanilla card
ActionSpec ResolveScript(const Card &card);      // derived + overlay merged
bool HasScript(uint32_t card_id);

}  // namespace openjoey::cards

#endif  // OPENJOEY_CARDS_CARD_API_H_
