#pragma once
#include "card/Card.hpp"
#include "ui/StyleSheet.hpp"
#include "card/ui/CardImageCache.hpp"
#include "card/ui/CardList.hpp"
#include "card/ui/Thumbnail.hpp"
#include <algorithm>
#include <raylib.h>
#include <string>
#include <vector>

// Scrollable 4-column card thumbnail grid. Replaces the old Grid widget.
namespace openjoey::ui {

struct CardGrid {
    static int ColCount() { return GRID_COLS; }

    static void Draw(const std::vector<const openjoey::Card*>& cards,
                     CardImageCache& cache,
                     int x, int y, int w, int h,
                     int cursor, bool focused) {
        const int cols    = GRID_COLS;
        const int gap     = GRID_GAP;
        const int labelH  = GRID_LABEL_H;
        const int cardW   = GRID_CARD_W(w);
        const int cardH   = GRID_CARD_H(w);
        const int cellH   = cardH + labelH + gap;
        const int selBdr  = SELECTION_BORDER;
        const int sbW     = SCROLLBAR_WIDTH;
        const int sbXOff  = SCROLLBAR_X_OFFSET;
        const int sbHTrim = SCROLLBAR_H_TRIM;
        const int sbThMin = SCROLLBAR_THUMB_MIN;

        int visRows   = std::max(1, h / cellH);
        int cursorRow = cards.empty() ? 0 : cursor / cols;
        int scrollRow = std::max(0, cursorRow - visRows / 2);

        for (int i = 0; i < (int)cards.size(); ++i) {
            int row    = i / cols;
            int col    = i % cols;
            int visRow = row - scrollRow;
            if (visRow < 0 || visRow >= visRows) continue;

            const auto& card  = *cards[i];
            bool        sel   = focused && i == cursor;
            Color       typeCol = CardList::cardTypeColor(card);

            int cx = x + gap + col * (cardW + gap);
            int cy = y + gap + visRow * cellH;
            Thumbnail::Draw(card, cache, cx, cy, cardW, cardH, typeCol);

            std::string name = card.name;
            while (!name.empty() &&
                   MeasureText(name.c_str(), FONT_CARD_STAT) > cardW)
                name.pop_back();
            if (name.size() < card.name.size()) name += "~";
            DrawText(name.c_str(), cx, cy + cardH + 2,
                     FONT_CARD_STAT, sel ? YELLOW : COLOR_STAT_TEXT);

            if (sel)
                DrawRectangleLines(cx - selBdr, cy - selBdr,
                                   cardW + selBdr * 2, cardH + selBdr * 2, YELLOW);
        }

        int totalRows = cards.empty() ? 0 : ((int)cards.size() + cols - 1) / cols;
        if (totalRows > visRows) {
            int   barH   = h - sbHTrim;
            int   barX   = x + w - sbXOff;
            float frac   = (float)scrollRow / std::max(1, totalRows - visRows);
            int   thumbH = std::max(sbThMin, barH * visRows / std::max(1, totalRows));
            int   thumbY = y + (int)(frac * (barH - thumbH));
            DrawRectangle(barX, y, sbW, barH, COLOR_SCROLLBAR_BG);
            DrawRectangle(barX, thumbY, sbW, thumbH, COLOR_SCROLLBAR_THUMB);
        }
    }
};

} // namespace openjoey::ui
