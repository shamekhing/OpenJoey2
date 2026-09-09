#pragma once
#include <algorithm>
#include <cstddef>
#include <fstream>
#include <iterator>
#include <string>
#include <unordered_map>
#include <vector>

#include "Card.hpp"
#include "CardParser.hpp"

namespace openjoey::cards {

// ── CardDatabase ─────────────────────────────────────────────────────────────
// Owns every parsed Card and hands out non-owning pointers into its storage.
// Pointers/references remain valid until the database is destroyed, moved
// from, or reloaded.
//
// Header-only by design: every member is defined inside the class body, so
// including this header from multiple translation units is ODR-safe.
//
// Movable, not copyable: copying would silently dangle the id/name index
// (they point into cards_' storage).
class CardDatabase {
   public:
    CardDatabase() = default;
    CardDatabase(const CardDatabase &) = delete;
    CardDatabase &operator=(const CardDatabase &) = delete;
    CardDatabase(CardDatabase &&) = default;
    CardDatabase &operator=(CardDatabase &&) = default;

    // Reads `path` from disk and loads it. On any failure the database is left
    // empty and false is returned.
    bool LoadFromFile(const std::string &path) {
        std::ifstream file(path);
        if (!file.is_open()) return false;
        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        return LoadFromString(content);
    }

    // Parses a remote card-data payload already held in memory (see
    // parseRemoteCardJson). On failure the database is left
    // empty and false is returned.
    bool LoadFromString(const std::string &content) {
        Clear();
        ParseResult parsed = parseRemoteCardJson(content);
        if (!parsed.ok()) return false;

        cards_ = std::move(parsed.cards);
        byId_.reserve(cards_.size());
        byName_.reserve(cards_.size());
        for (Card &c : cards_) {
            byId_[c.id] = &c;
            if (byName_.find(c.name) == byName_.end()) byName_[c.name] = &c;  // first card wins on duplicate names
        }
        return true;
    }

    void Clear() {
        cards_.clear();
        byId_.clear();
        byName_.clear();
    }

    std::size_t size() const { return cards_.size(); }
    bool empty() const { return cards_.empty(); }

    // ── Lookups (nullptr when not found) ───────────────────────────────────────

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

    // Substring search over card names
    std::vector<const Card *> FindByName(const std::string &name) const {
        std::vector<const Card *> out;
        for (const Card &c : cards_)
            if (c.name.find(name) != std::string::npos) out.push_back(&c);
        std::sort(out.begin(), out.end(), [](const Card *a, const Card *b) { return a->id < b->id; });
        return out;
    }

    // Attribute search over card attributes
    std::vector<const Card *> GetCardByAttribute(Attribute attr) const {
        std::vector<const Card *> out;
        for (const Card &c : cards_)
            if (c.hasAttribute(attr)) out.push_back(&c);
        std::sort(out.begin(), out.end(), [](const Card *a, const Card *b) { return a->id < b->id; });
        return out;
    }
    // Read-only access to the owned cards. Mutating the vector itself
    // (insert/erase/resize) would dangle the id/name index, so non-const
    // access to the container is deliberately not offered — mutate individual
    // cards through GetCardById / GetCardByName instead.
    const std::vector<Card> &GetAllCards() const { return cards_; }

   private:
    std::vector<Card> cards_;
    std::unordered_map<uint32_t, Card *> byId_;
    std::unordered_map<std::string, Card *> byName_;
};

}  // namespace openjoey::cards
