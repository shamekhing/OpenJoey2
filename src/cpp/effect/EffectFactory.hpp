#pragma once
#include "effect/Effect.hpp"
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

// Forward-include all concrete effect headers so EffectFactory is the single
// registration point. Add a new entry here when a new effect is created.
#include "effect/access/DrawEffect.hpp"
#include "effect/movement/DestroyEffect.hpp"
#include "effect/negate/NegateActivationEffect.hpp"
#include "effect/persistent/ContinuousEffect.hpp"
#include "effect/summon/NormalSummonEffect.hpp"

namespace openjoey {

/**
 * Creates a fresh Effect instance for each activation.
 *
 * Effects are not singletons because each activation can hold source card,
 * target, and parameter state. Chain owns the returned unique_ptr.
 */
class EffectFactory {
public:
  using Creator = std::function<std::unique_ptr<Effect>()>;

  EffectFactory() {
    // ── Access
    reg("draw_1", [] { return std::make_unique<DrawEffect>(1); });
    reg("draw_2", [] { return std::make_unique<DrawEffect>(2); });
    reg("draw_3", [] { return std::make_unique<DrawEffect>(3); });
    reg("draw_4", [] { return std::make_unique<DrawEffect>(4); });
    reg("draw_5", [] { return std::make_unique<DrawEffect>(5); });
    reg("draw_6", [] { return std::make_unique<DrawEffect>(6); });

    // ── Movement
    reg("destroy_target_opp_monster",
        [] { return std::make_unique<DestroyEffect>(DestroyEffect::Scope::OppMonster); });
    reg("destroy_target_any_monster",
        [] { return std::make_unique<DestroyEffect>(DestroyEffect::Scope::AnyMonster); });
    reg("destroy_target_any_st",
        [] { return std::make_unique<DestroyEffect>(DestroyEffect::Scope::AnySpellTrap); });
    reg("destroy_target_opp_st",
        [] { return std::make_unique<DestroyEffect>(DestroyEffect::Scope::OppSpellTrap); });

    // ── Summon
    reg("normal_summon", [] { return std::make_unique<NormalSummonEffect>(); });

    // ── Negate
    reg("negate_activation", [] { return std::make_unique<NegateActivationEffect>(); });

    // ── Persistent (ContinuousEffect is a base — register concrete subclasses here)
  }

  // Returns nullptr and logs if the key is unknown.
  std::unique_ptr<Effect> create(const std::string &key) const {
    auto it = table_.find(key);
    if (it == table_.end()) {
      std::cerr << "[EffectFactory] unknown key: " << key << "\n";
      return nullptr;
    }
    return it->second();
  }

  bool knows(const std::string &key) const { return table_.count(key) > 0; }

private:
  std::unordered_map<std::string, Creator> table_;

  void reg(const std::string &key, Creator fn) {
    table_[key] = std::move(fn);
  }
};

} // namespace openjoey
