#pragma once
#include "Card.hpp"

namespace openjoey::cards::compare {

// ── Ordering comparators for card collections ────────────────────────────────
// Every comparator is a strict weak ordering, suitable for std::sort. Each
// falls back to the name so the resulting order is total (stable output).
// Previously these lived on Card as static members; sorting is a UI concern
// and now lives here.

inline bool byId(const Card &a, const Card &b) { return a.id < b.id; }
inline bool byName(const Card &a, const Card &b) { return a.name < b.name; }
inline bool byLevel(const Card &a, const Card &b) { return a.level != b.level ? a.level < b.level : byName(a, b); }
inline bool byAtk(const Card &a, const Card &b) { return a.atk != b.atk ? a.atk < b.atk : byName(a, b); }
inline bool byDef(const Card &a, const Card &b) { return a.def != b.def ? a.def < b.def : byName(a, b); }

// Frame family order: Monster < Spell < Trap < anything else. The legacy Card
// surface has no single `type` member — derive it from the attribute list.
inline int frameRank(const Card &c) {
    if (c.hasAttribute(Attribute::Monster)) return 0;
    if (c.hasAttribute(Attribute::Spell)) return 1;
    if (c.hasAttribute(Attribute::Trap)) return 2;
    return 3;
}
inline bool byFrame(const Card &a, const Card &b) { return frameRank(a) != frameRank(b) ? frameRank(a) < frameRank(b) : byName(a, b); }

}  // namespace openjoey::cards::compare
