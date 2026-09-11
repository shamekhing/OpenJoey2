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
// ─────────────────────────────────────────────────────────────────────────────
// ───────────────────────────── CARD DATABASE ─────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// Owns every parsed Card and hands out non-owning pointers into its storage.
// Pointers/references remain valid until the database is destroyed, moved
// from, or reloaded.
//
// Movable, not copyable: copying would silently dangle the id/name index
// (they point into cards_' storage).
//
// Method declarations only — definitions follow below.
//
// Definition order (callee before caller):
//   Clear ← LoadFromString ← LoadFromFile
class CardDatabase {
    private:
        std::vector<Card> cards_;
        std::unordered_map<uint32_t, Card *> byId_;
        std::unordered_map<std::string, Card *> byName_;
        std::unordered_map<Attribute, std::vector<Card *>> byAttribute_;

    public:
        CardDatabase() = default;
        CardDatabase(const CardDatabase &) = delete;
        CardDatabase &operator=(const CardDatabase &) = delete;
        CardDatabase(CardDatabase &&) = default;
        CardDatabase &operator=(CardDatabase &&) = default;

        bool LoadFromFile(const std::string &path);
        bool LoadFromString(const std::string &content);

        void Clear();
        std::size_t size() const;
        bool empty() const;

        // ── Lookups (nullptr when not found) ────────────────────────────────
        Card *GetCardById(uint32_t id);
        const Card *GetCardById(uint32_t id) const;

        Card *GetCardByName(const std::string &name);
        const Card *GetCardByName(const std::string &name) const;

        // Substring search over card names
        std::vector<const Card *> FindByName(const std::string &name) const;

        // Attribute search over card attributes
        std::vector<const Card *> GetCardByAttribute(Attribute attr) const;

        // Read-only access to the owned cards. Mutating the vector itself
        // (insert/erase/resize) would dangle the id/name index, so non-const
        // access to the container is deliberately not offered — mutate individual
        // cards through GetCardById / GetCardByName instead.
        const std::vector<Card> &GetAllCards() const;
};

}  // namespace openjoey::cards
