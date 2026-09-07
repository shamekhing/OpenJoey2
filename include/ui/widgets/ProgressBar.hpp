#pragma once
#include <raylib.h>

#include <algorithm>

#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {

class ProgressBar {
   public:
    static void Draw(int x, int y, int w, int h, float frac) {
        Color fill = (frac >= 1.0f) ? GREEN : YELLOW;
        DrawRectangle(x, y, w, h, COLOR_PROGRESS_BG);
        DrawRectangle(x, y, (int)(w * std::min(frac, 1.0f)), h, fill);
        DrawRectangleLines(x, y, w, h, GRAY);
    }
};

}  // namespace openjoey::ui
