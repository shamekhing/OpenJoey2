#pragma once
// ── Deck-file IO (openjoey::ui) ──────────────────────────────────────────────
// One shared implementation for the two deck-file users (DeckEditorScreen
// save/load and the duel's default-deck bootstrap). Format: one numeric card
// id per line; blank lines and '#' comments are ignored.

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "cards/CardDatabase.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

struct DeckFile {
    // Write visible card ids, one per line. The caller creates directories.
    static void Write(const std::filesystem::path& path,
                      const std::vector<openjoey::cards::Card>& deck) {
        std::ofstream f(path);
        if (!f.is_open()) return;
        for (const auto& c : deck) f << c.id << "\n";
    }

    // Resolve ids against db; unknown ids and comment lines are skipped.
    // maxCards caps the result (<= 0 = unlimited). Never throws on bad lines.
    static std::vector<openjoey::cards::Card> Read(const std::filesystem::path& path,
                                                   const openjoey::cards::CardDatabase& db,
                                                   int maxCards = 0) {
        std::vector<openjoey::cards::Card> deck;
        std::ifstream f(path);
        if (!f.is_open()) return deck;
        std::string line;
        while (std::getline(f, line)) {
            if (line.empty() || line[0] == '#') continue;
            try {
                const uint32_t id = (uint32_t)std::stoul(line);
                if (const auto* card = db.GetCardById(id))
                    if (maxCards <= 0 || (int)deck.size() < maxCards)
                        deck.push_back(*card);
            } catch (...) {
            }
        }
        return deck;
    }
};

}  // namespace openjoey::ui