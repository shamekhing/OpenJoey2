#pragma once
// ── Deck-editor pool policy (openjoey::ui) ──────────────────────────────────
// Sort/filter policy for the card pool, deck-construction limits, and copy
// counting. Pure functions — no raylib, no state: the deck editor screen
// (screens/) drives them each frame.

#include <algorithm>
#include <cctype>
#include <string>
#include <utility>
#include <vector>

#include "cards/Card.hpp"
#include "cards/CardCompare.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

// ── Deck-construction limits (single source; the screen aliases these) ──────
struct DeckLimits {
    static constexpr int kMinDeckSize = 40;
    static constexpr int kMaxDeckSize = 60;
    static constexpr int kMaxCopies = 3;
};

enum class DeckSortMode {
    Type,
    NameDesc,
    NameAsc,
    LevelDesc,
    LevelAsc,
    AtkDesc,
    AtkAsc,
    DefDesc,
    DefAsc,
    Id,
    COUNT
};
enum class DeckTypeFilter { All,
                            Monster,
                            Spell,
                            Trap,
                            COUNT };

inline const char* sortModeLabel(DeckSortMode m) {
    static constexpr const char* kLabels[] = {
        "Type", "Name (A-Z)", "Name (Z-A)", "Level (desc)", "Level (asc)",
        "ATK (desc)", "ATK (asc)", "DEF (desc)", "DEF (asc)", "ID"};
    int idx = (int)m;
    return (idx >= 0 && idx < (int)DeckSortMode::COUNT) ? kLabels[idx] : "?";
}

inline const char* typeFilterLabel(DeckTypeFilter f) {
    static constexpr const char* kLabels[] = {"All", "Monster", "Spell", "Trap"};
    int idx = (int)f;
    return (idx >= 0 && idx < (int)DeckTypeFilter::COUNT) ? kLabels[idx] : "?";
}

// Sort `pool` in place according to `mode` (cards::compare comparators).
inline void sortPool(std::vector<openjoey::cards::Card>& pool, DeckSortMode mode) {
    using CmpFn = bool (*)(const openjoey::cards::Card&, const openjoey::cards::Card&);
    namespace cardcmp = openjoey::cards::compare;
    static constexpr std::pair<CmpFn, bool> kSort[] = {
        {cardcmp::byFrame, false},
        {cardcmp::byName, false},
        {cardcmp::byName, true},
        {cardcmp::byLevel, false},
        {cardcmp::byLevel, true},
        {cardcmp::byAtk, false},
        {cardcmp::byAtk, true},
        {cardcmp::byDef, false},
        {cardcmp::byDef, true},
        {cardcmp::byId, false},
    };
    auto [cmp, rev] = kSort[(int)mode];
    std::sort(pool.begin(), pool.end(), cmp);
    if (rev) std::reverse(pool.begin(), pool.end());
}

// Type filter + case-insensitive name query; returns pointers into `pool`.
inline std::vector<const openjoey::cards::Card*>
filterPool(const std::vector<openjoey::cards::Card>& pool, DeckTypeFilter type,
           const std::string& query) {
    std::vector<const openjoey::cards::Card*> out;
    std::string q = query;
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);
    for (const auto& c : pool) {
        if (type == DeckTypeFilter::Monster && !c.isMonster()) continue;
        if (type == DeckTypeFilter::Spell && !c.isSpell()) continue;
        if (type == DeckTypeFilter::Trap && !c.isTrap()) continue;
        if (!q.empty()) {
            std::string lo = c.name;
            std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
            if (lo.find(q) == std::string::npos) continue;
        }
        out.push_back(&c);
    }
    return out;
}

inline int countCopies(const std::vector<openjoey::cards::Card>& deck, uint32_t id) {
    int n = 0;
    for (const auto& c : deck)
        if (c.id == id) ++n;
    return n;
}

}  // namespace openjoey::ui
