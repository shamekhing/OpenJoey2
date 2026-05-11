#pragma once
#include "ui/core/Theme.hpp"
#include <raylib.h>

namespace openjoey::ui {

struct ScreenChrome {
    static void DrawHeader(int x, int y, int w, int h,
                           const char* title, const char* rightLabel,
                           Color titleColor, const Theme& t) {
        DrawRectangle(x, y, w, h, t.colors.headerBg);
        DrawLine(x, y + h - 1, x + w, y + h - 1, t.colors.dividerLine);
        int fs = t.fontScreenTitle;
        DrawText(title, x + t.headerTitleX, y + (h - fs) / 2, fs, titleColor);
        if (rightLabel && rightLabel[0]) {
            int tw = MeasureText(rightLabel, fs);
            DrawText(rightLabel, x + w - tw - t.headerTitleX,
                     y + (h - fs) / 2, fs, t.colors.statText);
        }
    }

    static void DrawFooter(int x, int y, int w, int h,
                           const char* helpText, const Theme& t) {
        DrawRectangle(x, y, w, h, t.colors.footerBg);
        DrawLine(x, y, x + w, y, t.colors.dividerLine);
        int fs = t.fontHelpText;
        DrawText(helpText, x + t.mainPadX, y + (h - fs) / 2, fs, t.colors.statText);
    }
};

} // namespace openjoey::ui
