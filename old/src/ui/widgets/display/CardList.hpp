#pragma once
#include "card/Card.hpp"
#include "ui/core/Theme.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include "ui/widgets/display/Thumbnail.hpp"
#include <functional>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

struct CardList {
    static void DrawItem(const openjoey::Card& card, CardImageCache& cache,
                         int x, int y, int w,
                         bool selected, int copies, int maxCopies) {
        const Theme t = Theme::FromScreen();
        Color typeCol = cardTypeColor(card);
        int txX = x + t.thumbnailPad, txY = y + t.thumbnailPad / 2;
        int txH = t.cardItemHeight - t.thumbnailPad;
        Thumbnail::Draw(card, cache, txX, txY, t.thumbnailWidth, txH, typeCol);

        int textX = txX + t.thumbnailWidth + t.thumbnailTextGap;
        DrawText(card.cardTypeTag().c_str(), textX, y + t.cardTypeY, t.fontCardType, typeCol);

        std::string stat = card.shortStat();
        if (!stat.empty())
            DrawText(stat.c_str(), textX, y + t.cardStatY, t.fontCardStat, t.colors.statText);

        std::string name = card.name;
        int maxNameW = w - textX - t.cardNameRightMargin;
        while (!name.empty() && MeasureText(name.c_str(), t.fontCardName) > maxNameW)
            name.pop_back();
        if (name.size() < card.name.size()) name += "~";
        DrawText(name.c_str(), textX, y + t.cardNameY, t.fontCardName, selected ? YELLOW : WHITE);

        Color cpCol = (copies >= maxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
        DrawText((std::to_string(copies) + "/3").c_str(),
                 x + w - t.copyCountXOffset, y + t.cardTypeY, t.fontCardType, cpCol);

        if (selected)
            DrawRectangleLines(x + t.selectionBorder, y,
                               w - t.selectionBorder * 2, t.cardItemHeight, YELLOW);
    }

    static void Draw(const std::vector<const openjoey::Card*>& cards,
                     CardImageCache& cache,
                     int x, int y, int w, int h,
                     int cursor, bool focused, int maxCopies,
                     std::function<int(uint32_t)> countFn) {
        const Theme t = Theme::FromScreen();
        int maxVis = h / t.cardItemHeight;
        int scroll = std::max(0, cursor - maxVis / 2);

        for (int i = 0; i < maxVis && scroll + i < (int)cards.size(); ++i) {
            int idx = scroll + i;
            DrawItem(*cards[idx], cache, x, y + i * t.cardItemHeight, w,
                     focused && idx == cursor, countFn(cards[idx]->cardNumber), maxCopies);
        }

        if ((int)cards.size() > maxVis) {
            int   barH   = h - t.scrollbarHTrim;
            int   barX   = x + w - t.scrollbarXOffset;
            float frac   = (float)scroll / std::max(1, (int)cards.size() - maxVis);
            int   thumbH = std::max(t.scrollbarThumbMin, barH * maxVis / std::max(1, (int)cards.size()));
            int   thumbY = y + (int)(frac * (barH - thumbH));
            DrawRectangle(barX, y, t.scrollbarWidth, barH, t.colors.scrollbarBg);
            DrawRectangle(barX, thumbY, t.scrollbarWidth, thumbH, t.colors.scrollbarThumb);
        }
    }

    static Color cardTypeColor(const openjoey::Card& c) {
        return c.isMonster() ? MAROON : (c.isSpell() ? GREEN : PINK);
    }
};

} // namespace openjoey::ui
