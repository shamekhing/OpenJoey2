#pragma once
#include "card/Card.hpp"
#include "ui/StyleSheet.hpp"
#include "card/ui/CardImageCache.hpp"
#include "card/ui/Thumbnail.hpp"
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

// Replaces the old List + ListItem pair. CardList renders a scrollable,
// cursor-tracked list of card rows with thumbnail, type tag, stat line,
// copy count, and selection border — all in one self-contained widget.
namespace openjoey::ui {

struct CardList {
    // Single card row renderer (was ListItem).
    static void DrawItem(const openjoey::Card& card, CardImageCache& cache,
                         int x, int y, int w,
                         bool selected, int copies, int maxCopies) {
        const int itemH   = CARD_ITEM_HEIGHT;
        const int txW     = THUMBNAIL_WIDTH;
        const int txPad   = THUMBNAIL_PAD;
        const int textGap = THUMBNAIL_TEXT_GAP;
        const int selBdr  = SELECTION_BORDER;
        const int cpXOff  = COPY_COUNT_X_OFFSET;
        const int nmRight = CARD_NAME_RIGHT_MARGIN;

        Color typeCol = cardTypeColor(card);
        int txX = x + txPad, txY = y + txPad / 2, txH = itemH - txPad;
        Thumbnail::Draw(card, cache, txX, txY, txW, txH, typeCol);

        int textX = txX + txW + textGap;
        DrawText(card.cardTypeTag().c_str(), textX, y + CARD_TYPE_Y, FONT_CARD_TYPE, typeCol);

        std::string stat = card.shortStat();
        if (!stat.empty())
            DrawText(stat.c_str(), textX, y + CARD_STAT_Y, FONT_CARD_STAT, COLOR_STAT_TEXT);

        std::string name = card.name;
        int maxNameW = w - textX - nmRight;
        while (!name.empty() && MeasureText(name.c_str(), FONT_CARD_NAME) > maxNameW)
            name.pop_back();
        if (name.size() < card.name.size()) name += "~";
        DrawText(name.c_str(), textX, y + CARD_NAME_Y, FONT_CARD_NAME, selected ? YELLOW : WHITE);

        Color cpCol = (copies >= maxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
        DrawText((std::to_string(copies) + "/3").c_str(),
                 x + w - cpXOff, y + CARD_TYPE_Y, FONT_CARD_TYPE, cpCol);

        if (selected)
            DrawRectangleLines(x + selBdr, y, w - selBdr * 2, itemH, YELLOW);
    }

    // Full scrollable list (was List::Draw).
    static void Draw(const std::vector<const openjoey::Card*>& cards,
                     CardImageCache& cache,
                     int x, int y, int w, int h,
                     int cursor, bool focused, int maxCopies,
                     std::function<int(uint32_t)> countFn) {
        const int itemH    = CARD_ITEM_HEIGHT;
        const int sbW      = SCROLLBAR_WIDTH;
        const int sbXOff   = SCROLLBAR_X_OFFSET;
        const int sbHTrim  = SCROLLBAR_H_TRIM;
        const int sbThMin  = SCROLLBAR_THUMB_MIN;

        int maxVis = h / itemH;
        int scroll = std::max(0, cursor - maxVis / 2);

        for (int i = 0; i < maxVis && scroll + i < (int)cards.size(); ++i) {
            int idx = scroll + i;
            DrawItem(*cards[idx], cache, x, y + i * itemH, w,
                     focused && idx == cursor, countFn(cards[idx]->cardId), maxCopies);
        }

        if ((int)cards.size() > maxVis) {
            int   barH   = h - sbHTrim;
            int   barX   = x + w - sbXOff;
            float frac   = (float)scroll / std::max(1, (int)cards.size() - maxVis);
            int   thumbH = std::max(sbThMin, barH * maxVis / std::max(1, (int)cards.size()));
            int   thumbY = y + (int)(frac * (barH - thumbH));
            DrawRectangle(barX, y, sbW, barH, COLOR_SCROLLBAR_BG);
            DrawRectangle(barX, thumbY, sbW, thumbH, COLOR_SCROLLBAR_THUMB);
        }
    }

    static Color cardTypeColor(const openjoey::Card& c) {
        return c.isMonster() ? MAROON : (c.isSpell() ? GREEN : PINK);
    }
};

} // namespace openjoey::ui
