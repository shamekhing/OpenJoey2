#pragma once
// ── Deck-editor panels (openjoey::ui) ───────────────────────────────────────
// The three deck-editor panel renderers (pool / preview / deck), mirroring the
// DuelPanels pattern: the struct holds const references to the editor's live
// state, so it always renders the current frame's data. All layout constants
// come from the uikit StyleSheet.

#include <raylib.h>

#include <string>
#include <ui/cards/CardGrid.hpp>
#include <ui/cards/CardImageCache.hpp>
#include <ui/cards/CardList.hpp>
#include <ui/cards/DeckStats.hpp>
#include <vector>

#include "cards/Card.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/deck/DeckFilters.hpp"
#include "ui/widgets/KeyboardNav.hpp"
#include "ui/widgets/Panel.hpp"
#include "ui/widgets/StyleSheet.hpp"
#include "ui/widgets/TextInput.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

struct DeckPanels {
    // Live editor state (references — always current).
    const AppContext& ctx;
    const TextInput& searchInput;
    const KeyboardNav& poolNav;
    const KeyboardNav& deckNav;
    const std::vector<openjoey::cards::Card>& deck;
    const DeckSortMode& sortMode;
    const DeckTypeFilter& typeFilter;
    const bool& focusPool;
    const bool& deckGridView;

    // ── Card Pool (search bar + list) ────────────────────────────────────────
    void drawPool(const std::vector<const openjoey::cards::Card*>& fp,
                  int x, int y, int w, int h) const {
        std::string badge = std::string(sortModeLabel(sortMode)) + "  [" +
                            typeFilterLabel(typeFilter) + "]  " +
                            std::to_string(fp.size()) + " cards";
        Panel::Draw("Card Pool", badge.empty() ? nullptr : badge.c_str(),
                    x, y, w, h, focusPool);

        int searchY = y + SEARCH_BAR_Y_OFFSET;
        searchInput.Draw(x + THUMBNAIL_PAD, searchY,
                         w - THUMBNAIL_PAD * 2, SEARCH_BAR_HEIGHT(h));

        int listY = searchY + LIST_Y_OFFSET;
        CardList::Draw(fp, ctx.imageCache, x, listY, w, y + h - listY,
                       poolNav.cursor, focusPool, DeckLimits::kMaxCopies,
                       [&](uint32_t id) { return countCopies(deck, id); });
    }

    // ── Preview (art, type line, copy count, wrapped description) ───────────
    void drawPreview(const std::vector<const openjoey::cards::Card*>& fp,
                     int x, int y, int w, int h) const {
        DrawRectangleLines(x, y, w, h, DARKGRAY);
        DrawText("Preview", x + PREVIEW_PAD_X, y + CARD_TYPE_Y,
                 FONT_CARD_NAME, DARKGRAY);

        const openjoey::cards::Card* card = nullptr;
        if (focusPool && !fp.empty() && poolNav.cursor < (int)fp.size())
            card = fp[poolNav.cursor];
        else if (!focusPool && !deck.empty() &&
                 deckNav.cursor < (int)deck.size())
            card = &deck[deckNav.cursor];
        if (!card) return;

        Color col = CardList::cardTypeColor(*card);
        int artX = x + PREVIEW_PAD_X;
        int artY = y + PREVIEW_ART_Y_OFFSET;
        int artW = w - PREVIEW_ART_SIDE_PAD;
        int artH = (int)((float)artW * PREVIEW_ASPECT_RATIO);
        if (artY + artH > y + h - MAIN_PAD_BOTTOM / 2)
            artH = y + h - MAIN_PAD_BOTTOM / 2 - artY;

        const Texture2D* tex = ctx.imageCache.Get(*card);
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
        DrawText(card->statLine().c_str(), x + PREVIEW_PAD_X, infoY,
                 FONT_CARD_TYPE, LIGHTGRAY);
        infoY += FONT_CARD_TYPE + PREVIEW_INFO_GAP;

        int copies = countCopies(deck, card->id);
        Color cpCol = (copies >= DeckLimits::kMaxCopies) ? RED
                      : (copies > 0)                     ? GREEN
                                                         : GRAY;
        DrawText(TextFormat("In deck: %d / %d", copies, DeckLimits::kMaxCopies),
                 x + PREVIEW_PAD_X, infoY, FONT_CARD_STAT, cpCol);
        infoY += FONT_CARD_STAT + PREVIEW_INFO_GAP;

        const std::string& desc = card->description;
        int charsPerLine =
            (w - PREVIEW_DESC_SIDE_PAD) / std::max(1, PREVIEW_DESC_CHAR_W);
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

    // ── Deck (stats header + grid/list) ──────────────────────────────────────
    void drawDeck(int x, int y, int w, int h) const {
        Color border = !focusPool ? YELLOW : DARKGRAY;
        DrawRectangleLines(x, y, w, h, border);
        std::string title = deckGridView ? "Deck [Grid]" : "Deck [List]";
        DrawText(title.c_str(), x + PREVIEW_PAD_X, y + CARD_TYPE_Y,
                 FONT_PANEL_TITLE, border);

        DeckStats::Draw(deck, DeckLimits::kMinDeckSize,
                        x + PREVIEW_PAD_X, y + SEARCH_BAR_Y_OFFSET,
                        w - PREVIEW_ART_SIDE_PAD);

        std::vector<const openjoey::cards::Card*> deckPtrs;
        deckPtrs.reserve(deck.size());
        for (const auto& c : deck) deckPtrs.push_back(&c);

        int listY = y + DECK_LIST_Y_OFFSET;
        int listH = y + h - listY;

        if (deckGridView) {
            CardGrid::Draw(deckPtrs, ctx.imageCache, x, listY, w, listH,
                           deckNav.cursor, !focusPool);
        } else {
            CardList::Draw(deckPtrs, ctx.imageCache, x, listY, w, listH,
                           deckNav.cursor, !focusPool, DeckLimits::kMaxCopies,
                           [&](uint32_t id) { return countCopies(deck, id); });
        }
    }
};

}  // namespace openjoey::ui
