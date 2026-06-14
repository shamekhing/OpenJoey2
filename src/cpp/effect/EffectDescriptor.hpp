#pragma once
#include <string>

namespace openjoey {

/**
 * Pure data descriptor for effect bindings supplied by adapters/data loaders.
 *
 * The key maps to a concrete Effect subclass through EffectFactory::create().
 */
struct EffectDescriptor {
  std::string key; // e.g. "draw_2", "negate_activation"
};

} // namespace openjoey
