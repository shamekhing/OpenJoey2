#pragma once
#include "ui/core/Theme.hpp"
#include <raylib.h>
#include <string>

namespace openjoey::ui {

struct Panel {
    static void Draw(const char* title, const char* badge,
                     int x, int y, int w, int h, bool focused) {
        const Theme t = Theme::FromScreen();
        Color border = focused ? YELLOW : DARKGRAY;
        DrawRectangleLines(x, y, w, h, border);

        int hdrH = t.searchBarYOffset;
        DrawRectangle(x, y, w, hdrH, t.colors.headerBg);

        int fs = t.fontPanelTitle;
        DrawText(title, x + t.previewPadX, y + (hdrH - fs) / 2, fs, border);

        if (badge && badge[0]) {
            int bw = MeasureText(badge, t.fontCardStat);
            DrawText(badge, x + w - bw - t.previewPadX,
                     y + (hdrH - t.fontCardStat) / 2,
                     t.fontCardStat, focused ? YELLOW : t.colors.statText);
        }
    }

    static void Draw(const char* title, int x, int y, int w, int h, bool focused) {
        Draw(title, nullptr, x, y, w, h, focused);
    }

    static Rectangle contentRect(int x, int y, int w, int h) {
        int hdrH = Theme::FromScreen().searchBarYOffset;
        return {(float)x, (float)(y + hdrH), (float)w, (float)(h - hdrH)};
    }
};

} // namespace openjoey::ui
