#pragma once
#include "card/Card.hpp"
#include "ui/core/Theme.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include "ui/widgets/display/CardList.hpp"
#include "ui/widgets/display/Thumbnail.hpp"
#include <algorithm>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

struct CardGrid {
    static int ColCount() { return 4; }

    static void Draw(const std::vector<const openjoey::Card*>& cards,
                     CardImageCache& cache,
                     int x, int y, int w, int h,
                     int cursor, bool focused) {
        const Theme t = Theme::FromScreen();
        const int cols   = t.gridCols;
        const int gap    = t.gridGap;
        const int labelH = t.gridLabelH;
        const int cardW  = t.gridCardW(w);
        const int cardH  = t.gridCardH(w);
        const int cellH  = cardH + labelH + gap;

        int visRows   = std::max(1, h / cellH);
        int cursorRow = cards.empty() ? 0 : cursor / cols;
        int scrollRow = std::max(0, cursorRow - visRows / 2);

        for (int i = 0; i < (int)cards.size(); ++i) {
            int row    = i / cols;
            int col    = i % cols;
            int visRow = row - scrollRow;
            if (visRow < 0 || visRow >= visRows) continue;

            const auto& card    = *cards[i];
            bool        sel     = focused && i == cursor;
            Color       typeCol = CardList::cardTypeColor(card);

            int cx = x + gap + col * (cardW + gap);
            int cy = y + gap + visRow * cellH;
            Thumbnail::Draw(card, cache, cx, cy, cardW, cardH, typeCol);

            std::string name = card.name;
            while (!name.empty() &&
                   MeasureText(name.c_str(), t.fontCardStat) > cardW)
                name.pop_back();
            if (name.size() < card.name.size()) name += "~";
            DrawText(name.c_str(), cx, cy + cardH + 2,
                     t.fontCardStat, sel ? YELLOW : t.colors.statText);

            if (sel)
                DrawRectangleLines(cx - t.selectionBorder, cy - t.selectionBorder,
                                   cardW + t.selectionBorder * 2,
                                   cardH + t.selectionBorder * 2, YELLOW);
        }

        int totalRows = cards.empty() ? 0 : ((int)cards.size() + cols - 1) / cols;
        if (totalRows > visRows) {
            int   barH   = h - t.scrollbarHTrim;
            int   barX   = x + w - t.scrollbarXOffset;
            float frac   = (float)scrollRow / std::max(1, totalRows - visRows);
            int   thumbH = std::max(t.scrollbarThumbMin, barH * visRows / std::max(1, totalRows));
            int   thumbY = y + (int)(frac * (barH - thumbH));
            DrawRectangle(barX, y, t.scrollbarWidth, barH, t.colors.scrollbarBg);
            DrawRectangle(barX, thumbY, t.scrollbarWidth, thumbH, t.colors.scrollbarThumb);
        }
    }
};

} // namespace openjoey::ui
