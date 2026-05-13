#pragma once
#include <string>

namespace openjoey {

// ─── EffectDescriptor ─────────────────────────────────────────────────────────
// Pure data — one effect entry as stored in effect_registry.json.
// The key maps to a concrete Effect subclass via EffectFactory::create().

struct EffectDescriptor {
  std::string key; // e.g. "draw_2", "negate_activation"
};

} // namespace openjoey
