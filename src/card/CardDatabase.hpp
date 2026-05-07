#pragma once
#include "card/Card.hpp"
#include "card/CardParser.hpp"
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace openjoey {

class CardDatabase {
public:
  bool LoadFromFile(const std::string &path);

  // ── Public API

  Card *GetCardById(uint32_t id) {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
  }

  const Card *GetCardById(uint32_t id) const {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
  }

  Card *GetCardByName(const std::string &name) {
    auto it = byName_.find(name);
    return it != byName_.end() ? it->second : nullptr;
  }

  const Card *GetCardByName(const std::string &name) const {
    auto it = byName_.find(name);
    return it != byName_.end() ? it->second : nullptr;
  }

  std::vector<const Card *> GetCardsByName(const std::string &name) const {
    std::vector<const Card *> out;
    for (const auto &[n, c] : byName_)
      if (n.find(name) != std::string::npos)
        out.push_back(c);
    return out;
  }

  std::vector<Card> &GetAllCards() { return cards_; }
  const std::vector<Card> &GetAllCards() const { return cards_; }

private:
  std::vector<Card> cards_;
  std::unordered_map<uint32_t, Card *> byId_;
  std::unordered_map<std::string, Card *> byName_;
};

} // namespace openjoey

bool openjoey::CardDatabase::LoadFromFile(const std::string &path) {
  std::ifstream file(path);
  if (!file.is_open())
    return false;

  std::string content((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());

  cards_.clear();
  byId_.clear();
  byName_.clear();

  if (!tryLoadYgoProDeckJson(content, &cards_))
    return false;

  byId_.reserve(cards_.size());
  byName_.reserve(cards_.size());

  for (Card &c : cards_) {
    byId_[c.cardNumber] = &c;
    if (byName_.find(c.name) == byName_.end())
      byName_[c.name] = &c;
  }

  return true;
}