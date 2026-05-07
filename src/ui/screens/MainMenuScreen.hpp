#pragma once
#include "ui/AppScreen.hpp"
#include "ui/StyleSheet.hpp"
#include <raylib.h>

namespace openjoey::ui {

static const char *kItems[] = {
    "Duel", "Deck Editor", "Settings", "Testing", "Quit",
};
static constexpr AppScreen kScreenMap[] = {
    AppScreen::Duel, AppScreen::DeckEditor, AppScreen::Settings,
    AppScreen::Testing, AppScreen::MainMenu};

class MainMenuScreen {
public:
  MainMenuScreen() : selected_(0), next_(AppScreen::MainMenu), quit_(false) {}

  void Update() {
    if (IsKeyPressed(KEY_DOWN))
      selected_ = (selected_ + 1) % kItemCount;
    if (IsKeyPressed(KEY_UP))
      selected_ = (selected_ - 1 + kItemCount) % kItemCount;
    if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
      if (selected_ == kItemCount - 1)
        quit_ = true;
      else
        next_ = kScreenMap[selected_];
    }
  }

  void Draw() const {
    const int sw = GetScreenWidth();
    const int sh = GetScreenHeight();

    ClearBackground(COLOR_BG_DARK);
    DrawRectangle(0, 0, sw, sh, Fade(BLACK, OVERLAY_FADE));

    const char *title = "OpenJoey";
    const int titleW = MeasureText(title, FONT_MAIN_TITLE);
    DrawText(title, (sw - titleW) / 2, sh / 4, FONT_MAIN_TITLE, GOLD);

    const int startY = sh / 2;
    for (int i = 0; i < kItemCount; ++i) {
      Color col = (i == selected_) ? YELLOW : LIGHTGRAY;
      int tw = MeasureText(kItems[i], FONT_MENU_ITEM);
      if (i == selected_)
        DrawText(">", (sw - tw) / 2 - MENU_ARROW_OFFSET,
                 startY + i * MENU_ITEM_SPACING, FONT_MENU_ITEM, YELLOW);
      DrawText(kItems[i], (sw - tw) / 2, startY + i * MENU_ITEM_SPACING,
               FONT_MENU_ITEM, col);
    }
    DrawText("UP/DOWN to navigate, ENTER to select", HEADER_TITLE_X,
             sh - MENU_HELP_BOTTOM_OFFSET, FONT_HELP_SMALL, DARKGRAY);
  }

  AppScreen NextScreen() const { return next_; }
  bool ShouldQuit() const { return quit_; }

private:
  int selected_ = 0;
  AppScreen next_ = AppScreen::MainMenu;
  bool quit_ = false;
  static constexpr int kItemCount = 5;
};

} // namespace openjoey::ui
