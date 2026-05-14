#pragma once
#include "Type.hpp"
#include "card/CardParser.hpp"
#include "card/ICardRepository.hpp"
#include <fstream>
#include <iostream>
#include <nlohmann/json.hpp>
#include <unordered_map>

namespace openjoey {

// ─── LocalCardRepository ─────────────────────────────────────────────────────
// In-process implementation of ICardRepository backed by a YGOProDeck-format
// cards.json file. All cards are held in memory; flush() rewrites the file.
//
// Swap for RemoteCardRepository when the card-DB microservice is ready —
// nothing else in the codebase changes.

class LocalCardRepository : public ICardRepository {
public:
  // ── ICardRepository — Read ──────────────────────────────────────────────

  const Card *getById(uint32_t id) const override {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
  }

  Card *getMutableById(uint32_t id) override {
    auto it = byId_.find(id);
    return it != byId_.end() ? it->second : nullptr;
  }

  const Card *getByName(const std::string &name) const override {
    auto it = byName_.find(name);
    return it != byName_.end() ? it->second : nullptr;
  }

  std::vector<const Card *> search(const CardFilter &f) const override {
    std::vector<const Card *> out;
    for (const Card &c : cards_) {
      if (f.type && c.type != *f.type)
        continue;
      if (f.nameContains && c.name.find(*f.nameContains) == std::string::npos)
        continue;
      if (f.levelMin && c.level < *f.levelMin)
        continue;
      if (f.levelMax && c.level > *f.levelMax)
        continue;
      if (f.atkMin && c.atk < *f.atkMin)
        continue;
      if (f.atkMax && c.atk > *f.atkMax)
        continue;
      out.push_back(&c);
    }
    return out;
  }

  const std::vector<Card> &all() const override { return cards_; }

  // ── ICardRepository — Write ─────────────────────────────────────────────

  bool add(Card card) override {
    if (byId_.count(card.cardNumber))
      return false;
    cards_.push_back(std::move(card));
    reindex();
    return true;
  }

  bool update(uint32_t id, Card card) override {
    auto it = byId_.find(id);
    if (it == byId_.end())
      return false;
    card.cardNumber = id; // keep id stable
    *it->second = std::move(card);
    reindex();
    return true;
  }

  bool remove(uint32_t id) override {
    auto it = byId_.find(id);
    if (it == byId_.end())
      return false;
    cards_.erase(
        std::find_if(cards_.begin(), cards_.end(),
                     [id](const Card &c) { return c.cardNumber == id; }));
    reindex();
    return true;
  }

  // ── Persistence ─────────────────────────────────────────────────────────

  bool loadFromFile(const std::string &path) override {
    filePath_ = path;
    std::ifstream f(path);
    if (!f.is_open()) {
      std::cerr << "[LocalCardRepository] cannot open: " << path << "\n";
      return false;
    }
    std::string content((std::istreambuf_iterator<char>(f)),
                        std::istreambuf_iterator<char>());
    cards_.clear();
    byId_.clear();
    byName_.clear();
    if (!tryLoadYgoProDeckJson(content, &cards_)) {
      std::cerr << "[LocalCardRepository] parse failed: " << path << "\n";
      return false;
    }
    reindex();
    return true;
  }

  // Rewrites the JSON file with the current in-memory state.
  bool flush() override {
    if (filePath_.empty()) {
      std::cerr << "[LocalCardRepository] flush: no file path set\n";
      return false;
    }
    nlohmann::json root;
    root["data"] = nlohmann::json::array();
    for (const Card &c : cards_) {
      nlohmann::json j;
      j["id"] = c.cardNumber;
      j["name"] = c.name;
      j["desc"] = c.description;
      j["frameType"] = c.type == enum_card::Spell  ? "spell"
                       : c.type == enum_card::Trap ? "trap"
                                                   : "effect";
      j["atk"] = c.atk;
      j["def"] = c.def;
      j["level"] = c.level;
      root["data"].push_back(std::move(j));
    }
    std::ofstream out(filePath_);
    if (!out.is_open()) {
      std::cerr << "[LocalCardRepository] flush: cannot write: " << filePath_
                << "\n";
      return false;
    }
    out << root.dump(2);
    return true;
  }

private:
  std::vector<Card> cards_;
  std::unordered_map<uint32_t, Card *> byId_;
  std::unordered_map<std::string, Card *> byName_;
  std::string filePath_;

  void reindex() {
    byId_.clear();
    byName_.clear();
    byId_.reserve(cards_.size());
    byName_.reserve(cards_.size());
    for (Card &c : cards_) {
      byId_[c.cardNumber] = &c;
      byName_.try_emplace(c.name, &c); // first card with that name wins
    }
  }
};

} // namespace openjoey
