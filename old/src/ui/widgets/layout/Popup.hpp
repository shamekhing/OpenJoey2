#pragma once
#include "ui/core/Theme.hpp"
#include <raylib.h>

namespace openjoey::ui {

struct Popup {
    // Dims the full screen, draws a centered popup box, returns the inner content rect.
    static Rectangle Begin(int pw, int ph, const char* title,
                           Color accentColor, const Theme& t) {
        int px = (t.sw - pw) / 2;
        int py = (t.sh - ph) / 2;
        DrawRectangle(0, 0, t.sw, t.sh, t.colors.overlayDim);
        DrawRectangle(px, py, pw, ph, t.colors.popupBg);
        DrawRectangleLinesEx({(float)px, (float)py, (float)pw, (float)ph},
                             2.f, accentColor);
        int pad     = t.previewPadX;
        int titleFs = t.fontPanelTitle;
        DrawText(title, px + pad, py + pad, titleFs, accentColor);
        return {(float)px,
                (float)(py + titleFs + pad * 2),
                (float)pw,
                (float)(ph - titleFs - pad * 3)};
    }

    static void End() {} // exists for symmetry
};

} // namespace openjoey::ui
