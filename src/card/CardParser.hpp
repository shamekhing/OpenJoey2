#pragma once
#include "card/Card.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_set>

namespace openjoey {
// ── YGOProDeck API (`{ "data": [ { ... }, ... ] }`)

static int parseStatField(const nlohmann::json &j, const char *key) {
  if (!j.contains(key))
    return 0;
  const auto &v = j.at(key);
  if (v.is_null())
    return 0;
  if (v.is_number_integer())
    return static_cast<int>(v.get<int>());
  if (v.is_number_unsigned())
    return static_cast<int>(v.get<unsigned>());
  if (v.is_number_float())
    return static_cast<int>(v.get<double>());
  if (v.is_string()) {
    const std::string s = v.get<std::string>();
    if (s == "?" || s.empty())
      return 0;
    try {
      return std::stoi(s);
    } catch (...) {
      return 0;
    }
  }
  return 0;
}

static int optIntMember(const nlohmann::json &j, const char *key, int def = 0) {
  auto it = j.find(key);
  if (it == j.end() || it->is_null() || !it->is_number())
    return def;
  if (it->is_number_unsigned())
    return static_cast<int>(it->get<uint64_t>());
  if (it->is_number_integer())
    return static_cast<int>(it->get<int64_t>());
  return static_cast<int>(it->get<double>());
}

static std::string optStringMember(const nlohmann::json &j, const char *key,
                                   const std::string &def = {}) {
  auto it = j.find(key);
  if (it == j.end() || it->is_null() || !it->is_string())
    return def;
  return it->get<std::string>();
}

static uint32_t optCardId(const nlohmann::json &j) {
  auto it = j.find("id");
  if (it == j.end() || it->is_null())
    return 0;
  if (it->is_number_unsigned())
    return static_cast<uint32_t>(it->get<uint64_t>());
  if (it->is_number_integer()) {
    const auto v = it->get<int64_t>();
    if (v <= 0)
      return 0;
    return static_cast<uint32_t>(v);
  }
  return 0;
}

static Card cardFromYgoProDeckJson(const nlohmann::json &j) {
  Card c;
  c.cardNumber = optCardId(j);
  c.name = optStringMember(j, "name");
  c.description = optStringMember(j, "desc");
  c.imageId = c.cardNumber;

  const std::string frame = optStringMember(j, "frameType");

  if (frame == "spell" || frame == "skill")
    c.type = CardType::Spell;
  else if (frame == "trap")
    c.type = CardType::Trap;
  else
    c.type = CardType::Monster;

  c.atk   = parseStatField(j, "atk");
  c.def   = parseStatField(j, "def");
  c.level = optIntMember(j, "level", 0);
  const int rank = optIntMember(j, "rank", 0);
  if (c.level == 0 && rank > 0)
    c.level = rank;

  return c;
}

static bool tryLoadYgoProDeckJson(const std::string &content,
                                  std::vector<Card> *out) {
  try {
    const auto root = nlohmann::json::parse(content);
    if (!root.is_object() || !root.contains("data") ||
        !root.at("data").is_array())
      return false;
    std::unordered_set<uint32_t> seenIds;
    for (const auto &item : root.at("data")) {
      if (!item.is_object())
        continue;
      try {
        Card card = cardFromYgoProDeckJson(item);
        if (card.cardNumber == 0)
          continue;
        if (card.name.empty())
          card.name = "Card " + std::to_string(card.cardNumber);
        if (!seenIds.insert(card.cardNumber).second)
          continue;
        out->push_back(std::move(card));
      } catch (const std::exception &) {
        // Skip malformed entries; keep loading the rest of the database.
      }
    }
    return !out->empty();
  } catch (const nlohmann::json::exception &ex) {
    std::cerr << "[CardParser] YGOProDeck JSON: " << ex.what() << "\n";
    return false;
  } catch (const std::exception &ex) {
    std::cerr << "[CardParser] JSON load: " << ex.what() << "\n";
    return false;
  }
}
} // namespace openjoey