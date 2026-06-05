#pragma once
#include "card/Card.hpp"
#include "ui/core/Theme.hpp"
#include "ui/widgets/display/ProgressBar.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

class DeckStats {
public:
    static void Draw(const std::vector<openjoey::Card>& deck,
                     int minSize, int x, int y, int w) {
        const Theme t = Theme::FromScreen();
        int mon = 0, spl = 0, trp = 0;
        for (const auto& c : deck) {
            if (c.isMonster())    ++mon;
            else if (c.isSpell()) ++spl;
            else                  ++trp;
        }
        int total = (int)deck.size();
        Color okCol = (total >= minSize) ? GREEN : YELLOW;
        DrawText(TextFormat("%d/%d", total, minSize), x, y, t.fontDeckStats, okCol);
        ProgressBar::Draw(x, y + 18, w, 8, (float)total / minSize);
        y += 32;
        DrawText(("MON " + std::to_string(mon)).c_str(), x,                    y, t.fontCardStat, t.colors.monsterStat);
        DrawText(("SPL " + std::to_string(spl)).c_str(), x + t.statSplXOffset, y, t.fontCardStat, t.colors.spellStat);
        DrawText(("TRP " + std::to_string(trp)).c_str(), x + t.statTrpXOffset, y, t.fontCardStat, t.colors.trapStat);
    }
};

} // namespace openjoey::ui
