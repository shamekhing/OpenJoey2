#pragma once
#include "card/EffectID.hpp"
#include <cstdint>

namespace openjoey {

// ── Effect timing (Rulebook p.9 "Effect Monsters") ───────────────────────────
enum class EffectType : uint8_t {
    Ignition,     // activate during your Main Phase        (Spell Speed 1)
    Trigger,      // "when X happens" (includes Flip)
    Quick,        // Spell Speed 2 or higher
    Continuous,   // active while face-up on the field
    Cost,         // paid before activation (Tribute/Discard/Pay-LP...)
};

// ── A single subscribed effect on a Card ────────────────────────────────────
// A Card's identity is the set of EffectIDs it carries (Card.hpp).  The
// resolver in field/EffectResolver.hpp turns each id into concrete zone
// moves.  Keeping this struct in card/ (layer 1) lets Card.hpp stay in layer 1
// with NO dependency on a later layer — the layering invariant holds.
struct CardEffect {
    EffectID   id     = EffectID::None;
    EffectType timing = EffectType::Ignition;
    uint8_t    speed  = 1;     // Spell Speed: 1 / 2 / 3
        int        costLP = 0;     // LP cost when timing == Cost (0 = none)
};

} // namespace openjoey
