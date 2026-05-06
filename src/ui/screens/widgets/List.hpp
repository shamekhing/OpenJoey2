#include "game/ContentPaths.hpp"
#include <raylib.h>

static void DrawList() const {

  const int sw = GetScreenWidth();
  const int sh = GetScreenHeight();

  Image menu_bgImg = LoadImageRaw(
      openjoey::ContentPaths::BackgroundImage().string().c_str(), sw, sh, 4);
  ImageClearBackground(&menu_bgImg, Color{10, 10, 20, 255});
  DrawRectangle(0, 0, sw, sh, Fade(BLACK, 0.35f));

  const char *title = "OpenJoey";
  const int titleFontSize = 48;
  const int titleW = MeasureText(title, titleFontSize);
  DrawText(title, (sw - titleW) / 2, sh / 4, titleFontSize, GOLD);

  const int itemFontSize = 28;
  const int startY = sh / 2;
  for (int i = 0; i < kItemCount; ++i) {
    Color col = (i == selected_) ? YELLOW : LIGHTGRAY;
    int tw = MeasureText(kItems[i], itemFontSize);
    if (i == selected_)
      DrawText(">", (sw - tw) / 2 - 24, startY + i * 40, itemFontSize, YELLOW);
    DrawText(kItems[i], (sw - tw) / 2, startY + i * 40, itemFontSize, col);
  }
  DrawText("UP/DOWN to navigate, ENTER to select", 10, sh - 30, 16, DARKGRAY);
}