#pragma once
// ── Duel bootstrap (openjoey::ui) ────────────────────────────────────────────
// Deck construction (editor selection / default deck file / database
// fallback), extra-deck split, classic-effect wiring, seating into the
// engine, and the shared card-back texture. Stateless — DuelScreen owns the
// deck vectors (they back the card memory for the whole duel).

#include <raylib.h>

#include <filesystem>
#include <iostream>
#include <vector>

#include "cards/Card.hpp"
#include "cards/CardDatabase.hpp"
#include "engine/action/Catalog.hpp"
#include "engine/duel/Engine.hpp"
#include "engine/field/Field.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/deck/DeckFile.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

struct DuelSetup {
    static constexpr int kMaxDeckCards = 60;   // mirrors DeckEditorScreen::kMaxDeckSize
    static constexpr int kFallbackCards = 40;  // first-DB-cards fallback size

    // Load the shared card-back texture from the settings paths.
    static Texture2D loadCardBack(const AppContext& ctx) {
        Texture2D back{};
        const auto& path = ctx.settings.paths.cardBackImg;
        if (std::filesystem::exists(path)) back = LoadTexture(path.c_str());
        if (!back.id) std::cerr << "Failed to load card back image from " << path << "\n";
        return back;
    }

    // Starter deck for duels launched outside the deck editor: the settings
    // base dir's decks/default.txt (card ids, one per line — the format
    // DeckEditorScreen writes/reads). Unknown ids and comment lines are
    // ignored; empty when the file is missing or matches nothing (caller
    // falls back to the first DB cards).
    static std::vector<openjoey::cards::Card> loadDefaultDeck(const AppContext& ctx) { return DeckFile::Read(ctx.settings.baseDir_ / "decks" / "default.txt", ctx.cardDb, kMaxDeckCards); }

    // Both duelists play the same deck (hotseat): the deck editor's selection
    // when the player launched with [F], else decks/default.txt (the
    // classic-effect starter), else the first DB cards so a duel can always
    // start. Cards are value copies per player; the vectors are never resized
    // after this, so the raw zone pointers stay valid for the duel.
    static void buildDecks(const AppContext& ctx, std::vector<openjoey::cards::Card>& mainA, std::vector<openjoey::cards::Card>& mainB, std::vector<openjoey::cards::Card>& extraA, std::vector<openjoey::cards::Card>& extraB) {
        std::vector<openjoey::cards::Card> src;
        if (!ctx.selectedDeck.empty()) {
            src = ctx.selectedDeck;
        } else {
            src = loadDefaultDeck(ctx);
            if (src.empty()) {  // no saved deck: first DB cards
                for (auto& c : ctx.cardDb.GetAllCards()) {
                    src.push_back(c);
                    if ((int)src.size() >= kFallbackCards) break;
                }
            }
        }

        // Effect wiring: the engine resolves effects from the name-keyed
        // classic catalog (engine/action/Catalog.hpp) at activation time —
        // nothing to attach to the cards here.

        // Fusion monsters route to each player's Extra Deck.
        auto splitExtra = [](const std::vector<openjoey::cards::Card>& in, std::vector<openjoey::cards::Card>& main, std::vector<openjoey::cards::Card>& extra) {
            for (auto& c : in) (c.isExtraDeckMonster() ? extra : main).push_back(c);
        };

        mainA.clear();
        mainB.clear();
        extraA.clear();
        extraB.clear();
        splitExtra(src, mainA, extraA);  // player 1
        splitExtra(src, mainB, extraB);  // player 2 (same deck, hotseat)
    }

    // Seat main decks via the engine (shuffled there) and copy extra decks
    // straight into their zones (the engine reads the ExtraDeck zone for
    // Fusion Summons; Cyber-Stein special-summons from it too).
    static void seatDecks(Engine& engine, zone::Field& field, std::vector<openjoey::cards::Card>& mainA, std::vector<openjoey::cards::Card>& mainB, std::vector<openjoey::cards::Card>& extraA, std::vector<openjoey::cards::Card>& extraB) {
        for (int p = 0; p < 2; ++p) {
            auto& main = (p == 0) ? mainA : mainB;
            std::vector<Card*> ptrs;
            for (auto& c : main) ptrs.push_back(&c);
            engine.setDeck(p, ptrs);
            engine.sealDeckBacking(p, &main);  // seal the OWNING vector (ptrs is a temp)
            for (auto& c : (p == 0 ? extraA : extraB)) { field.extraDeckZones[p].put(&c); }
        }
        engine.shuffleDecks();
    }
};

}  // namespace openjoey::ui