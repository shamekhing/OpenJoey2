#pragma once
#include "card/Card.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/screens/widgets/Thumbnail.hpp"
#include <raylib.h>
#include <string>

namespace openjoey::ui {

class ListItem {
public:
  static void Draw(const openjoey::Card &card, CardImageCache &cache,
                   int x, int y, int w,
                   bool selected, int copies, int maxCopies) {
    // Cache screen-relative metrics once per item draw
    const int itemH    = CARD_ITEM_HEIGHT;
    const int txW      = THUMBNAIL_WIDTH;
    const int txPad    = THUMBNAIL_PAD;
    const int textGap  = THUMBNAIL_TEXT_GAP;
    const int selBdr   = SELECTION_BORDER;
    const int cpXOff   = COPY_COUNT_X_OFFSET;
    const int nmRight  = CARD_NAME_RIGHT_MARGIN;

    Color typeCol = cardTypeColor(card);
    int txX = x + txPad;
    int txY = y + txPad / 2;
    int txH = itemH - txPad;
    Thumbnail::Draw(card, cache, txX, txY, txW, txH, typeCol);

    int textX = txX + txW + textGap;
    DrawText(card.cardTypeTag().c_str(), textX, y + CARD_TYPE_Y,
             FONT_CARD_TYPE, typeCol);

    std::string stat = card.shortStat();
    if (!stat.empty())
      DrawText(stat.c_str(), textX, y + CARD_STAT_Y,
               FONT_CARD_STAT, COLOR_STAT_TEXT);

    std::string name = card.name;
    int maxNameW = w - textX - nmRight;
    while (!name.empty() && MeasureText(name.c_str(), FONT_CARD_NAME) > maxNameW)
      name.pop_back();
    if (name.size() < card.name.size())
      name += "~";
    DrawText(name.c_str(), textX, y + CARD_NAME_Y,
             FONT_CARD_NAME, selected ? YELLOW : WHITE);

    Color cpCol = (copies >= maxCopies) ? RED : (copies > 0 ? GREEN : GRAY);
    DrawText((std::to_string(copies) + "/3").c_str(),
             x + w - cpXOff, y + CARD_TYPE_Y,
             FONT_CARD_TYPE, cpCol);

    if (selected)
      DrawRectangleLines(x + selBdr, y,
                         w - selBdr * 2, itemH, YELLOW);
  }

  static Color cardTypeColor(const openjoey::Card &c) {
    return c.isMonster() ? MAROON : (c.isSpell() ? GREEN : PINK);
  }
};

} // namespace openjoey::ui
