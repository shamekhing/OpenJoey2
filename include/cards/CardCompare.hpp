#pragma once
#include "Card.hpp"

namespace openjoey::cards::compare {
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────── CARD COMPARATORS ───────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// Ordering comparators for card collections.
// Every comparator is a strict weak ordering, suitable for std::sort. Each
// falls back to the name so the resulting order is total (stable output).
//
// Definition order (callee before caller):
//   byName ← byLevel, byAtk, byDef, byAttribute
//
// byName has no dependencies on other comparators; it is called by the rest.
inline bool byName(const Card &a, const Card &b) { return a.name < b.name; }
inline bool byId(const Card &a, const Card &b)   { return a.id < b.id; }
inline bool byLevel(const Card &a, const Card &b) { return a.level != b.level ? a.level < b.level : byName(a, b); }
inline bool byAtk(const Card &a, const Card &b)    { return a.atk != b.atk ? a.atk < b.atk : byName(a, b); }
inline bool byDef(const Card &a, const Card &b)    { return a.def != b.def ? a.def < b.def : byName(a, b); }
inline bool byAttribute(const Card &a, const Card &b, Attribute attr) {
    const bool aHas = a.hasAttribute(attr);
    const bool bHas = b.hasAttribute(attr);
    return aHas != bHas ? aHas : byName(a, b);
}
}  // namespace openjoey::cards::compare
