#pragma once
#include "card/ICardRepository.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <vector>

namespace openjoey {

// ─── EffectRegistry ──────────────────────────────────────────────────────────
// Loads effect_registry.json and populates card.effectKeys on each card in the
// repository. JSON format (version 2):
//
//   { "version": 2, "bindings": { "<passcode>": ["key1", "key2"], ... } }
//
// bind() must be called after the repository is loaded.

class EffectRegistry {
public:
  // Load bindings from file.
  bool loadFromFile(const std::string &path) {
    std::ifstream f(path);
    if (!f.is_open()) {
      std::cerr << "[EffectRegistry] cannot open: " << path << "\n";
      return false;
    }
    try {
      const auto root = nlohmann::json::parse(f);
      if (!root.contains("bindings") || !root.at("bindings").is_object()) {
        std::cerr << "[EffectRegistry] missing 'bindings' object\n";
        return false;
      }
      bindings_.clear();
      for (auto &[idStr, keysJson] : root.at("bindings").items()) {
        if (!keysJson.is_array())
          continue;
        uint32_t id = 0;
        try {
          id = static_cast<uint32_t>(std::stoul(idStr));
        } catch (const std::exception &ex) {
          std::cerr << "[EffectRegistry] bad card id '" << idStr << "': " << ex.what() << "\n";
          continue;
        }
        std::vector<std::string> keys;
        for (const auto &k : keysJson)
          if (k.is_string())
            keys.push_back(k.get<std::string>());
        bindings_[id] = std::move(keys);
      }
      return true;
    } catch (const std::exception &ex) {
      std::cerr << "[EffectRegistry] parse error: " << ex.what() << "\n";
      return false;
    }
  }

  // Populate card.effectKeys for every card in the repository whose
  // id appears in bindings_. Clears existing effectKeys first.
  void bind(ICardRepository &repo) const {
    for (auto &[id, keys] : bindings_) {
      Card *c = repo.getMutableById(id);
      if (!c) {
        std::cerr << "[EffectRegistry] no card for id " << id << "\n";
        continue;
      }
      c->effectKeys = keys;
    }
  }

  // Raw access to the parsed bindings (useful for debugging / server sync).
  const std::unordered_map<uint32_t, std::vector<std::string>> &
  bindings() const {
    return bindings_;
  }

private:
  std::unordered_map<uint32_t, std::vector<std::string>> bindings_;
};

} // namespace openjoey
