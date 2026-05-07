#pragma once
#include "card/Card.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/screens/widgets/ListItem.hpp"
#include <functional>
#include <raylib.h>
#include <vector>

namespace openjoey::ui {

class List {
public:
  static void Draw(const std::vector<const openjoey::Card *> &cards,
                   CardImageCache &cache,
                   int x, int y, int w, int h,
                   int cursor, bool focused, int maxCopies,
                   std::function<int(uint32_t)> countFn) {
    // Cache metrics once per draw call
    const int itemH    = CARD_ITEM_HEIGHT;
    const int sbW      = SCROLLBAR_WIDTH;
    const int sbXOff   = SCROLLBAR_X_OFFSET;
    const int sbHTrim  = SCROLLBAR_H_TRIM;
    const int sbThMin  = SCROLLBAR_THUMB_MIN;

    int maxVis = h / itemH;
    int scroll = std::max(0, cursor - maxVis / 2);

    for (int i = 0; i < maxVis && scroll + i < (int)cards.size(); ++i) {
      int idx = scroll + i;
      const auto &card = *cards[idx];
      bool sel = focused && idx == cursor;
      int copies = countFn(card.cardNumber);
      ListItem::Draw(card, cache, x, y + i * itemH, w, sel, copies, maxCopies);
    }

    if ((int)cards.size() > maxVis) {
      int barH   = h - sbHTrim;
      int barX   = x + w - sbXOff;
      float frac = (float)scroll / std::max(1, (int)cards.size() - maxVis);
      int thumbH = std::max(sbThMin, barH * maxVis / std::max(1, (int)cards.size()));
      int thumbY = y + (int)(frac * (barH - thumbH));
      DrawRectangle(barX, y, sbW, barH, COLOR_SCROLLBAR_BG);
      DrawRectangle(barX, thumbY, sbW, thumbH, COLOR_SCROLLBAR_THUMB);
    }
  }
};

} // namespace openjoey::ui
