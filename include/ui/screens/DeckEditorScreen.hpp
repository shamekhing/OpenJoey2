#pragma once
// ── Deck editor screen ──────────────────────────────────────────────────────
// Orchestrator + input machine only, mirroring the DuelScreen split:
//   • sort/filter policy → deck/DeckFilters.hpp (pure functions + limits)
//   • panel rendering    → deck/DeckPanels.hpp  (pool / preview / deck)
//   • deck-file IO       → deck/DeckFile.hpp    (shared with duel bootstrap)

#include "cards/Card.hpp"
#include "cards/CardDatabase.hpp"
#include <ui/cards/CardGrid.hpp>
#include "ui/AppScreen.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/deck/DeckFile.hpp"
#include "ui/deck/DeckFilters.hpp"
#include "ui/deck/DeckPanels.hpp"
#include "ui/widgets/StyleSheet.hpp"
#include "ui/widgets/KeyboardNav.hpp"
#include "ui/widgets/TextInput.hpp"
#include <filesystem>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

class DeckEditorScreen : public IScreen {
public:
    // Deck-construction limits live in DeckLimits (single source).
    static constexpr int kMinDeckSize = DeckLimits::kMinDeckSize;
    static constexpr int kMaxDeckSize = DeckLimits::kMaxDeckSize;
    static constexpr int kMaxCopies   = DeckLimits::kMaxCopies;

    explicit DeckEditorScreen(AppContext& ctx)
        : ctx_(ctx),
          panels_{ctx_, searchInput_, poolNav_, deckNav_, deck_,
                  sortMode_, typeFilter_, focusPool_, deckGridView_} {
        rebuildPool();
        if (ctx_.cardDb.GetAllCards().empty())
            statusMsg_ = "No cards loaded — check data/cards.json";
    }

    ScreenEvent Update(float /*dt*/) override;
    void        Draw() const override;

    bool SaveDeck(const std::string& name) const;
    bool LoadDeck(const std::string& name);

private:
    AppContext& ctx_;

    std::vector<openjoey::cards::Card> pool_;
    std::vector<openjoey::cards::Card> deck_;

    TextInput   searchInput_;
    bool        focusPool_    = true;
    bool        deckGridView_ = false;
    KeyboardNav poolNav_;
    KeyboardNav deckNav_;

    DeckSortMode   sortMode_   = DeckSortMode::Type;
    DeckTypeFilter typeFilter_ = DeckTypeFilter::All;

    std::string statusMsg_;

    // References the members above — declare (and construct) last.
    DeckPanels panels_;

    void rebuildPool();
};

// ── Pool rebuild

inline void DeckEditorScreen::rebuildPool() {
    pool_.clear();
    for (const auto& c : ctx_.cardDb.GetAllCards())
        pool_.push_back(c);
    sortPool(pool_, sortMode_);
}

// ── Update (input machine)

inline ScreenEvent DeckEditorScreen::Update(float /*dt*/) {
    ctx_.imageCache.PollAndLoad();

    if (IsKeyPressed(KEY_ESCAPE) && !searchInput_.isTyping())
        return ScreenEvent::replace(AppScreen::MainMenu);

    searchInput_.Update();
    if (searchInput_.isChanged()) poolNav_.cursor = 0;

    if (IsKeyPressed(KEY_O)) {
        sortMode_ = static_cast<DeckSortMode>(
            (static_cast<int>(sortMode_) + 1) % static_cast<int>(DeckSortMode::COUNT));
        rebuildPool();
        poolNav_.cursor = 0;
        statusMsg_ = std::string("Sort: ") + sortModeLabel(sortMode_);
    }
    if (IsKeyPressed(KEY_T)) {
        typeFilter_ = static_cast<DeckTypeFilter>(
            (static_cast<int>(typeFilter_) + 1) % static_cast<int>(DeckTypeFilter::COUNT));
        poolNav_.cursor = 0;
        statusMsg_ = std::string("Filter: ") + typeFilterLabel(typeFilter_);
    }

    auto fp    = filterPool(pool_, typeFilter_, searchInput_.GetText());
    int poolSz = (int)fp.size();
    int deckSz = (int)deck_.size();

    if (focusPool_) {
        poolNav_.setCount(poolSz);
        poolNav_.handleClampKeys();

        if (IsKeyPressed(KEY_ENTER) && poolSz > 0) {
            const auto& card = *fp[poolNav_.cursor];
            if ((int)deck_.size() < kMaxDeckSize &&
                countCopies(deck_, card.id) < kMaxCopies) {
                deck_.push_back(card);
                statusMsg_ = "Added: " + card.name;
            } else if (countCopies(deck_, card.id) >= kMaxCopies) {
                statusMsg_ = "Max 3 copies of " + card.name;
            } else {
                statusMsg_ = "Deck full (60 cards max)";
            }
        }
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_RIGHT))
            focusPool_ = false;

    } else {
        const int step = deckGridView_ ? CardGrid::ColCount() : 1;
        deckNav_.setCount(deckSz);

        if (IsKeyPressed(KEY_DOWN))      deckNav_.clampNext(step);
        if (IsKeyPressed(KEY_UP))        deckNav_.clampPrev(step);
        if (IsKeyPressed(KEY_PAGE_DOWN)) deckNav_.clampNext(step * 3);
        if (IsKeyPressed(KEY_PAGE_UP))   deckNav_.clampPrev(step * 3);

        if (deckGridView_) {
            if (IsKeyPressed(KEY_RIGHT)) deckNav_.clampNext();
            if (IsKeyPressed(KEY_LEFT))  deckNav_.clampPrev();
        }

        if ((IsKeyPressed(KEY_DELETE) || IsKeyPressed(KEY_BACKSPACE) ||
             IsKeyPressed(KEY_D)) && deckSz > 0) {
            statusMsg_ = "Removed: " + deck_[deckNav_.cursor].name;
            deck_.erase(deck_.begin() + deckNav_.cursor);
            deckNav_.setCount((int)deck_.size());
        }
        if (IsKeyPressed(KEY_G)) {
            deckGridView_ = !deckGridView_;
            statusMsg_ = deckGridView_ ? "Deck: grid view" : "Deck: list view";
        }
        if (IsKeyPressed(KEY_TAB) || (!deckGridView_ && IsKeyPressed(KEY_LEFT)))
            focusPool_ = true;
    }

    if (IsKeyPressed(KEY_S)) {
        SaveDeck("default");
        statusMsg_ = "Saved as 'default'";
    }
    if (IsKeyPressed(KEY_L)) {
        statusMsg_ = LoadDeck("default") ? "Loaded 'default'" : "No saved deck";
    }
    if (IsKeyPressed(KEY_C)) {
        deck_.clear();
        deckNav_.cursor = 0;
        statusMsg_ = "Deck cleared";
    }
    if (IsKeyPressed(KEY_F)) {
        if ((int)deck_.size() >= kMinDeckSize) {
            ctx_.selectedDeck = deck_;
            return ScreenEvent::replace(AppScreen::Duel);
        }
        statusMsg_ = "Need " +
                     std::to_string(kMinDeckSize - (int)deck_.size()) +
                     " more cards";
    }
    if (IsKeyPressed(KEY_ESCAPE))
        statusMsg_.clear();

    return ScreenEvent::none();
}

// ── Draw (layout + delegation to DeckPanels)

inline void DeckEditorScreen::Draw() const {
    ClearBackground(COLOR_BG_MAIN);
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, HEADER_HEIGHT, COLOR_HEADER_BG);
    DrawText("DECK EDITOR", HEADER_TITLE_X, HEADER_TITLE_Y, FONT_SCREEN_TITLE, WHITE);
    DrawText("[ESC] clear status", sw - HELP_TEXT_X_OFFSET, HELP_TEXT_Y, FONT_CARD_TYPE, GRAY);

    const int padY   = MAIN_PAD_Y;
    const int padX   = MAIN_PAD_X;
    const int padBot = MAIN_PAD_BOTTOM;
    const int panH   = sh - padY - padBot;
    const int poolW  = sw * POOL_WIDTH_PERCENT / 100;
    const int prevW  = sw * PREVIEW_WIDTH_PERCENT / 100;
    const int deckW  = sw - poolW - prevW - padX * 2;

    auto fp = filterPool(pool_, typeFilter_, searchInput_.GetText());
    panels_.drawPool(fp, padX, padY, poolW, panH);
    panels_.drawPreview(fp, padX + poolW, padY, prevW, panH);
    panels_.drawDeck(padX + poolW + prevW, padY, deckW, panH);

    DrawRectangle(0, sh - padBot, sw, padBot, COLOR_FOOTER_BG);
    const int barY = sh - padBot + STATUS_BAR_Y_OFFSET;
    DrawText("[TAB] switch  [Arrows] navigate  [PgUp/Dn] fast scroll  "
             "[ENTER] add  [DEL/D] remove  [O] sort  [T] filter  "
             "[G] grid/list  [C] clear  [S] save  [L] load  [F] duel (40+)",
             PREVIEW_PAD_X, barY, FONT_HELP_TEXT, LIGHTGRAY);
    if (!statusMsg_.empty())
        DrawText(statusMsg_.c_str(), PREVIEW_PAD_X, barY + FONT_HELP_TEXT + 2,
                 FONT_CARD_NAME, GREEN);
}

// ── Persistence

// Decks live under the settings base dir (beside the executable — the build dir
// symlinks openjoey-content/data), so save/load work from any working dir.
inline bool DeckEditorScreen::SaveDeck(const std::string& name) const {
    std::filesystem::path dir = ctx_.settings.baseDir_ / "decks";
    std::error_code ec;
    std::filesystem::create_directories(dir, ec);
    DeckFile::Write(dir / (name + ".txt"), deck_);
    return true;
}

inline bool DeckEditorScreen::LoadDeck(const std::string& name) {
    std::filesystem::path path = ctx_.settings.baseDir_ / "decks" / (name + ".txt");
    if (!std::filesystem::exists(path)) {
        statusMsg_ = "Deck not found: " + path.string();
        return false;
    }
    // Parse into a temp deck first: a broken/empty file must not wipe the
    // deck currently being edited.
    std::vector<openjoey::cards::Card> loaded = DeckFile::Read(path, ctx_.cardDb, kMaxDeckSize);
    if (loaded.empty()) {
        statusMsg_ = "Deck '" + name + "' is empty (or no IDs matched the DB)";
        return false;
    }
    deck_ = std::move(loaded);
    deckNav_.cursor = 0;
    statusMsg_ = "Loaded " + std::to_string(deck_.size()) + " cards: " + name;
    return true;
}

} // namespace openjoey::ui
