#pragma once
#include "card/Card.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/screens/widgets/ProgressBar.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

class DeckStats {
public:
  static void Draw(const std::vector<openjoey::Card> &deck,
                   int minSize, int x, int y, int w) {
    int mon = 0, spl = 0, trp = 0;
    for (const auto &c : deck) {
      if (c.isMonster())      ++mon;
      else if (c.isSpell())   ++spl;
      else                    ++trp;
    }
    int total = (int)deck.size();
    Color okCol = (total >= minSize) ? GREEN : YELLOW;
    DrawText(TextFormat("%d/%d", total, minSize), x, y, FONT_DECK_STATS, okCol);
    ProgressBar::Draw(x, y + 18, w, 8, (float)total / minSize);
    y += 32;
    DrawText(("MON " + std::to_string(mon)).c_str(), x, y,
             FONT_CARD_STAT, COLOR_MONSTER_STAT);
    DrawText(("SPL " + std::to_string(spl)).c_str(), x + STAT_SPL_X_OFFSET, y,
             FONT_CARD_STAT, COLOR_SPELL_STAT);
    DrawText(("TRP " + std::to_string(trp)).c_str(), x + STAT_TRP_X_OFFSET, y,
             FONT_CARD_STAT, COLOR_TRAP_STAT);
  }
};

} // namespace openjoey::ui
