#pragma once
#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/screens/widgets/DeckStats.hpp"
#include "ui/screens/widgets/Grid.hpp"
#include "ui/screens/widgets/Header.hpp"
#include "ui/screens/widgets/List.hpp"
#include "ui/screens/widgets/ListItem.hpp"
#include "ui/screens/widgets/TextInput.hpp"
#include "ui/widgets/input/KeyboardNav.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

enum class DeckSortMode {
    Type, NameDesc, NameAsc, LevelDesc, LevelAsc,
    AtkDesc, AtkAsc, DefDesc, DefAsc, Id, COUNT
};
enum class DeckTypeFilter { All, Monster, Spell, Trap, COUNT };

class DeckEditorScreen : public IScreen {
public:
    static constexpr int kMinDeckSize = 40;
    static constexpr int kMaxDeckSize = 60;
    static constexpr int kMaxCopies   = 3;

    explicit DeckEditorScreen(AppContext& ctx) : ctx_(ctx) {
        rebuildPool();
    }

    ScreenEvent Update(float /*dt*/) override;
    void        Draw() const override;

    bool SaveDeck(const std::string& name) const;
    bool LoadDeck(const std::string& name);

private:
    AppContext& ctx_;

    std::vector<openjoey::Card> pool_;
    std::vector<openjoey::Card> deck_;

    TextInput   searchInput_;
    bool        focusPool_    = true;
    bool        deckGridView_ = false;
    KeyboardNav poolNav_;
    KeyboardNav deckNav_;

    DeckSortMode   sortMode_   = DeckSortMode::Type;
    DeckTypeFilter typeFilter_ = DeckTypeFilter::All;

    std::string statusMsg_;

    void rebuildPool();
    std::vector<const openjoey::Card*> filteredPool() const;
    int  countInDeck(uint32_t id) const;

    void drawPoolPanel(const std::vector<const openjoey::Card*>& fp,
                       int x, int y, int w, int h) const;
    void drawPreviewPanel(const std::vector<const openjoey::Card*>& fp,
                          int x, int y, int w, int h) const;
    void drawDeckPanel(int x, int y, int w, int h) const;

    static const char* sortModeLabel(DeckSortMode m);
    static const char* typeFilterLabel(DeckTypeFilter f);
};

// ── Pool rebuild

inline void DeckEditorScreen::rebuildPool() {
    using CmpFn = bool (*)(const openjoey::Card&, const openjoey::Card&);
    pool_.clear();
    for (const auto& c : ctx_.cardDb.GetAllCards())
        pool_.push_back(c);

    static constexpr std::pair<CmpFn, bool> kSort[] = {
        {openjoey::Card::sortByCardType, false},
        {openjoey::Card::sortByName,     false},
        {openjoey::Card::sortByName,     true},
        {openjoey::Card::sortByLevel,    false},
        {openjoey::Card::sortByLevel,    true},
        {openjoey::Card::sortByAtk,      false},
        {openjoey::Card::sortByAtk,      true},
        {openjoey::Card::sortByDef,      false},
        {openjoey::Card::sortByDef,      true},
        {openjoey::Card::sortById,       false},
    };
    auto [cmp, rev] = kSort[(int)sortMode_];
    std::sort(pool_.begin(), pool_.end(), cmp);
    if (rev) std::reverse(pool_.begin(), pool_.end());
}

// ── Static label methods

inline const char* DeckEditorScreen::sortModeLabel(DeckSortMode m) {
    static constexpr const char* kLabels[] = {
        "Type", "Name (A-Z)", "Name (Z-A)", "Level (desc)", "Level (asc)",
        "ATK (desc)", "ATK (asc)", "DEF (desc)", "DEF (asc)", "ID"};
    int idx = (int)m;
    return (idx >= 0 && idx < (int)DeckSortMode::COUNT) ? kLabels[idx] : "?";
}

inline const char* DeckEditorScreen::typeFilterLabel(DeckTypeFilter f) {
    static constexpr const char* kLabels[] = {"All", "Monster", "Spell", "Trap"};
    int idx = (int)f;
    return (idx >= 0 && idx < (int)DeckTypeFilter::COUNT) ? kLabels[idx] : "?";
}

// ── Update

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

    auto fp    = filteredPool();
    int poolSz = (int)fp.size();
    int deckSz = (int)deck_.size();

    if (focusPool_) {
        poolNav_.setCount(poolSz);
        poolNav_.handleClampKeys();

        if (IsKeyPressed(KEY_ENTER) && poolSz > 0) {
            const auto& card = *fp[poolNav_.cursor];
            if ((int)deck_.size() < kMaxDeckSize &&
                countInDeck(card.cardNumber) < kMaxCopies) {
                deck_.push_back(card);
                statusMsg_ = "Added: " + card.name;
            } else if (countInDeck(card.cardNumber) >= kMaxCopies) {
                statusMsg_ = "Max 3 copies of " + card.name;
            } else {
                statusMsg_ = "Deck full (60 cards max)";
            }
        }
        if (IsKeyPressed(KEY_TAB) || IsKeyPressed(KEY_RIGHT))
            focusPool_ = false;

    } else {
        const int step = deckGridView_ ? Grid::ColCount() : 1;
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

// ── Filtered pool

inline std::vector<const openjoey::Card*>
DeckEditorScreen::filteredPool() const {
    std::vector<const openjoey::Card*> out;
    std::string q = searchInput_.GetText();
    std::transform(q.begin(), q.end(), q.begin(), ::tolower);

    for (const auto& c : pool_) {
        if (typeFilter_ == DeckTypeFilter::Monster && !c.isMonster()) continue;
        if (typeFilter_ == DeckTypeFilter::Spell   && !c.isSpell())   continue;
        if (typeFilter_ == DeckTypeFilter::Trap    && !c.isTrap())    continue;
        if (!q.empty()) {
            std::string lo = c.name;
            std::transform(lo.begin(), lo.end(), lo.begin(), ::tolower);
            if (lo.find(q) == std::string::npos) continue;
        }
        out.push_back(&c);
    }
    return out;
}

inline int DeckEditorScreen::countInDeck(uint32_t id) const {
    int n = 0;
    for (const auto& c : deck_)
        if (c.cardNumber == id) ++n;
    return n;
}

// ── Draw helpers

inline void DeckEditorScreen::drawPoolPanel(
    const std::vector<const openjoey::Card*>& fp, int x, int y, int w, int h) const {
    std::string badge = std::string(sortModeLabel(sortMode_)) + "  [" +
                        typeFilterLabel(typeFilter_) + "]  " +
                        std::to_string(fp.size()) + " cards";
    Header::Draw("Card Pool", badge, x, y, w, h, focusPool_);

    int searchY = y + SEARCH_BAR_Y_OFFSET;
    searchInput_.Draw(x + THUMBNAIL_PAD, searchY,
                      w - THUMBNAIL_PAD * 2, SEARCH_BAR_HEIGHT(h));

    int listY = searchY + LIST_Y_OFFSET;
    List::Draw(fp, ctx_.imageCache, x, listY, w, y + h - listY,
               poolNav_.cursor, focusPool_, kMaxCopies,
               [this](uint32_t id) { return countInDeck(id); });
}

inline void DeckEditorScreen::drawPreviewPanel(
    const std::vector<const openjoey::Card*>& fp, int x, int y, int w, int h) const {
    using namespace openjoey::ui;
    DrawRectangleLines(x, y, w, h, DARKGRAY);
    DrawText("Preview", x + PREVIEW_PAD_X, y + CARD_TYPE_Y, FONT_CARD_NAME, DARKGRAY);

    const openjoey::Card* card = nullptr;
    if (focusPool_ && !fp.empty() && poolNav_.cursor < (int)fp.size())
        card = fp[poolNav_.cursor];
    else if (!focusPool_ && !deck_.empty() && deckNav_.cursor < (int)deck_.size())
        card = &deck_[deckNav_.cursor];
    if (!card) return;

    Color col  = ListItem::cardTypeColor(*card);
    int artX   = x + PREVIEW_PAD_X;
    int artY   = y + PREVIEW_ART_Y_OFFSET;
    int artW   = w - PREVIEW_ART_SIDE_PAD;
    int artH   = (int)((float)artW * PREVIEW_ASPECT_RATIO);
    if (artY + artH > y + h - MAIN_PAD_BOTTOM / 2)
        artH = y + h - MAIN_PAD_BOTTOM / 2 - artY;

    const Texture2D* tex = ctx_.imageCache.Get(*card);
    Rectangle dst = {(float)artX, (float)artY, (float)artW, (float)artH};
    if (tex && tex->id != 0) {
        DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                       dst, {0, 0}, 0, WHITE);
    } else {
        DrawRectangle(artX, artY, artW, artH,
                      Color{col.r, col.g, col.b, CARD_PREVIEW_ALPHA});
        int nameW = MeasureText(card->name.c_str(), FONT_CARD_NAME);
        DrawText(card->name.c_str(), artX + (artW - nameW) / 2,
                 artY + artH / 2 - FONT_CARD_NAME / 2, FONT_CARD_NAME, WHITE);
    }
    DrawRectangleLines(artX, artY, artW, artH, col);
    DrawText(card->cardTypeTag().c_str(), artX + THUMBNAIL_PAD,
             artY + THUMBNAIL_PAD, FONT_CARD_STAT, col);

    int infoY = artY + artH + PREVIEW_INFO_GAP;
    DrawText(card->statLine().c_str(), x + PREVIEW_PAD_X, infoY, FONT_CARD_TYPE, LIGHTGRAY);
    infoY += FONT_CARD_TYPE + PREVIEW_INFO_GAP;

    int copies = countInDeck(card->cardNumber);
    Color cpCol = (copies >= kMaxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
    DrawText(TextFormat("In deck: %d / %d", copies, kMaxCopies),
             x + PREVIEW_PAD_X, infoY, FONT_CARD_STAT, cpCol);
    infoY += FONT_CARD_STAT + PREVIEW_INFO_GAP;

    const std::string& desc = card->description;
    int charsPerLine = (w - PREVIEW_DESC_SIDE_PAD) / std::max(1, PREVIEW_DESC_CHAR_W);
    int cur = 0;
    int lineH = FONT_CARD_STAT + 3;
    while (cur < (int)desc.size() && infoY < y + h - THUMBNAIL_PAD) {
        int end = std::min(cur + charsPerLine, (int)desc.size());
        if (end < (int)desc.size()) {
            int wb = (int)desc.rfind(' ', end);
            if (wb > cur) end = wb;
        }
        DrawText(desc.substr(cur, end - cur).c_str(),
                 x + PREVIEW_PAD_X, infoY, FONT_CARD_STAT, COLOR_DESC_TEXT);
        infoY += lineH;
        cur = end;
        if (cur < (int)desc.size() && desc[cur] == ' ') ++cur;
    }
}

inline void DeckEditorScreen::drawDeckPanel(int x, int y, int w, int h) const {
    Color border = !focusPool_ ? YELLOW : DARKGRAY;
    DrawRectangleLines(x, y, w, h, border);
    std::string title = deckGridView_ ? "Deck [Grid]" : "Deck [List]";
    DrawText(title.c_str(), x + PREVIEW_PAD_X, y + CARD_TYPE_Y, FONT_PANEL_TITLE, border);

    DeckStats::Draw(deck_, kMinDeckSize,
                    x + PREVIEW_PAD_X, y + SEARCH_BAR_Y_OFFSET,
                    w - PREVIEW_ART_SIDE_PAD);

    std::vector<const openjoey::Card*> deckPtrs;
    deckPtrs.reserve(deck_.size());
    for (const auto& c : deck_) deckPtrs.push_back(&c);

    int listY = y + DECK_LIST_Y_OFFSET;
    int listH = y + h - listY;

    if (deckGridView_) {
        Grid::Draw(deckPtrs, ctx_.imageCache, x, listY, w, listH,
                   deckNav_.cursor, !focusPool_);
    } else {
        List::Draw(deckPtrs, ctx_.imageCache, x, listY, w, listH,
                   deckNav_.cursor, !focusPool_, kMaxCopies,
                   [this](uint32_t id) { return countInDeck(id); });
    }
}

// ── Draw

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

    auto fp = filteredPool();
    drawPoolPanel(fp, padX, padY, poolW, panH);
    drawPreviewPanel(fp, padX + poolW, padY, prevW, panH);
    drawDeckPanel(padX + poolW + prevW, padY, deckW, panH);

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

inline bool DeckEditorScreen::SaveDeck(const std::string& name) const {
    std::filesystem::path dir =
        std::filesystem::current_path() / "data" / "decks";
    std::filesystem::create_directories(dir);
    std::ofstream f(dir / (name + ".txt"));
    if (!f.is_open()) return false;
    for (const auto& c : deck_)
        f << c.cardNumber << "\n";
    return true;
}

inline bool DeckEditorScreen::LoadDeck(const std::string& name) {
    std::filesystem::path path =
        std::filesystem::current_path() / "data" / "decks" / (name + ".txt");
    std::ifstream f(path);
    if (!f.is_open()) return false;
    deck_.clear();
    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#') continue;
        try {
            uint32_t id      = (uint32_t)std::stoul(line);
            const auto* card = ctx_.cardDb.GetCardById(id);
            if (card && (int)deck_.size() < kMaxDeckSize)
                deck_.push_back(*card);
        } catch (...) {}
    }
    deckNav_.cursor = 0;
    return !deck_.empty();
}

} // namespace openjoey::ui
