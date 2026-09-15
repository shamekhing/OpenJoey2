#pragma once
// ── Undo (duel/Undo.hpp) — snapshot / restore for one-step (bounded) undo ───
// A Duel snapshot deep-copies only what the engine owns:
//   * zones            — copied (they hold raw Card*; external deck-owned
//                        cards keep their addresses, so those pointers copy
//                        as-is)
//   * tokens           — DEEP-copied (unique_ptr), then every zone pointer to
//                        an old token is remapped to the clone (replacePtr)
//   * CardState of the referenced external cards — snapshotted separately and
//                        re-applied on restore (equips/counters/turn flags
//                        live on the shared card objects)
// turnState and chain link args are remapped for tokens as well.
#include <map>
#include <memory>
#include <vector>

#include "engine/duel/Duel.hpp"

namespace openjoey::engine {

// Deep-clone a Duel. `cardStates` (optional) collects the CardState of every
// non-token card referenced by the field, for restore-time re-application.
Duel cloneDuel(const Duel &src, std::map<Card *, cards::CardState> *cardStates = nullptr);

// One undo step. `duel` is the live state; `cardStates` re-applies the state
// of the shared external cards captured at checkpoint time.
struct DuelSnapshot {
    Duel duel;
    std::map<Card *, cards::CardState> cardStates;
};

DuelSnapshot makeSnapshot(const Duel &d);

void restoreSnapshot(Duel &d, const DuelSnapshot &s);

}  // namespace openjoey::engine