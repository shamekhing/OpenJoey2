#pragma once
#include "ui/StyleSheet.hpp"
#include <raylib.h>
#include <string>

namespace openjoey::ui {

class Header {
public:
  // Draw a panel header bar with title left-aligned and an optional badge right-aligned.
  static void Draw(const std::string &title, const std::string &badge,
                   int x, int y, int w, int h, bool focused) {
    const int fontSize = PANEL_HEADER_FONT(h);
    const int padX     = HEADER_TITLE_X;
    const int padY     = HEADER_TITLE_Y;
    Color col = COLOR_FOCUS(focused);
    DrawRectangleLines(x, y, w, h, col);
    DrawText(title.c_str(), x + padX, y + padY, fontSize, col);
    if (!badge.empty())
      DrawText(badge.c_str(),
               x + ALLIGN_RIGHT(badge.c_str(), w, fontSize),
               y + padY, fontSize, COLOR_TEXT);
  }

  // Overload without badge
  static void Draw(const std::string &title, int x, int y, int w, int h,
                   bool focused) {
    Draw(title, "", x, y, w, h, focused);
  }
};

} // namespace openjoey::ui
