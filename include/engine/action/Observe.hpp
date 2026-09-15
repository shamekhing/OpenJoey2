#pragma once
// ── act/Observe — read-only state view + legal action space ──────────────────
// The AI seams (foundation ai/Ai.hpp): RL player consumes Observe +
// LegalActions + the act:: functions. Both raylib-free.
#include <array>

#include "engine/action/Battle.hpp"
#include "engine/action/Chain.hpp"
#include "engine/action/Query.hpp"
#include "engine/action/Summon.hpp"

namespace openjoey::engine::action {

struct CardView {
    uint32_t cardId = 0;
    std::string name = "?";
    int atk = 0, def = 0;
    bool faceUp = false, token = false;
};

struct SideView {
    std::vector<CardView> monsters, spellsTraps, hand;
    int handCount = 0, deckCount = 0, extraDeckCount = 0, gyCount = 0, banishedCount = 0;
};

struct StateView {
    int turnPlayer = 0, turnNumber = 0;
    Phase phase = Phase::Draw;
    std::array<int, 2> lp{};
    std::array<SideView, 2> sides;
};

CardView MakeView(Card *c, bool known, bool faceUp);

StateView Observe(const Duel &d, int viewer);

// Legal action space: verb ids only, no card knowledge. Which specific card
// to act with is the caller's pick (the UI grid / the RL policy).
std::vector<ActionSpec> LegalActions(const Duel &d, int player);

}  // namespace openjoey::engine::action
