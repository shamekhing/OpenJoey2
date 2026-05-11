#pragma once
#include "ui/StyleSheet.hpp"
#include <raylib.h>
#include <string>

// Titled bordered panel — draws a header bar with optional right-aligned badge
// and a body background. Replaces the ad-hoc header+border pattern in screens.
namespace openjoey::ui {

struct Panel {
    // Draw a full panel: background + header bar + optional badge.
    static void Draw(const char* title, const char* badge,
                     int x, int y, int w, int h, bool focused) {
        Color border = focused ? YELLOW : DARKGRAY;
        DrawRectangleLines(x, y, w, h, border);

        int hdrH = SEARCH_BAR_Y_OFFSET;
        DrawRectangle(x, y, w, hdrH, COLOR_HEADER_BG);

        int fs = FONT_PANEL_TITLE;
        DrawText(title, x + PREVIEW_PAD_X, y + (hdrH - fs) / 2, fs, border);

        if (badge && badge[0]) {
            int bw = MeasureText(badge, FONT_CARD_STAT);
            DrawText(badge, x + w - bw - PREVIEW_PAD_X,
                     y + (hdrH - FONT_CARD_STAT) / 2,
                     FONT_CARD_STAT, focused ? YELLOW : COLOR_STAT_TEXT);
        }
    }

    static void Draw(const char* title, int x, int y, int w, int h, bool focused) {
        Draw(title, nullptr, x, y, w, h, focused);
    }

    // Returns the rectangle below the header where content should be drawn.
    static Rectangle contentRect(int x, int y, int w, int h) {
        int hdrH = SEARCH_BAR_Y_OFFSET;
        return {(float)x, (float)(y + hdrH), (float)w, (float)(h - hdrH)};
    }
};

} // namespace openjoey::ui
