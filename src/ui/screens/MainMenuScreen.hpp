#pragma once
#include "ui/core/AppContext.hpp"
#include "ui/core/AppScreen.hpp"
#include "ui/core/Theme.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/input/KeyboardNav.hpp"
#include <cassert>
#include <filesystem>
#include <raylib.h>

namespace openjoey::ui {

class MainMenuScreen : public IScreen {
public:
    explicit MainMenuScreen(AppContext& /*ctx*/) { loadBackground(); }

    ~MainMenuScreen() override {
        if (background_.id) UnloadTexture(background_);
    }

    ScreenEvent Update(float /*dt*/) override {
        nav_.setCount(kItemCount);
        nav_.handleWrapKeys();
        if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
            if (nav_.cursor == kItemCount - 1) return ScreenEvent::quit();
            return ScreenEvent::replace(kScreenMap[nav_.cursor]);
        }
        return ScreenEvent::none();
    }

    void Draw() const override {
        const Theme t = Theme::FromScreen();

        ClearBackground(t.colors.bgDark);
        if (background_.id) {
            const float scaleX = (float)t.sw / background_.width;
            const float scaleY = (float)t.sh / background_.height;
            DrawTextureEx(background_, {0.f, 0.f}, 0.f, fmaxf(scaleX, scaleY), WHITE);
        }
        DrawRectangle(0, 0, t.sw, t.sh, Fade(BLACK, 0.35f));

        const char* title  = "OpenJoey";
        const int   titleW = MeasureText(title, t.fontMainTitle);
        DrawText(title, (t.sw - titleW) / 2, t.sh / 4, t.fontMainTitle, GOLD);

        const int startY = t.sh / 2;
        for (int i = 0; i < kItemCount; ++i) {
            Color col = (i == nav_.cursor) ? YELLOW : LIGHTGRAY;
            int tw = MeasureText(kItems[i], t.fontMenuItem);
            if (i == nav_.cursor)
                DrawText(">", (t.sw - tw) / 2 - t.menuArrowOffset,
                         startY + i * t.menuItemSpacing, t.fontMenuItem, YELLOW);
            DrawText(kItems[i], (t.sw - tw) / 2,
                     startY + i * t.menuItemSpacing, t.fontMenuItem, col);
        }
        DrawText("UP/DOWN to navigate, ENTER to select", t.headerTitleX,
                 t.sh - t.menuHelpBottomOffset, t.fontHelpSmall, DARKGRAY);
    }

private:
    static constexpr int kItemCount = 5;
    static constexpr const char* kItems[kItemCount] = {
        "Duel", "Deck Editor", "Settings", "Testing", "Quit",
    };
    static constexpr AppScreen kScreenMap[kItemCount] = {
        AppScreen::Duel, AppScreen::DeckEditor,
        AppScreen::Settings, AppScreen::Testing, AppScreen::MainMenu,
    };
    static_assert(sizeof(kItems)     / sizeof(kItems[0]) ==
                  sizeof(kScreenMap) / sizeof(kScreenMap[0]),
                  "kItems and kScreenMap must have the same length");

    KeyboardNav nav_;
    Texture2D   background_ = {};

    void loadBackground() {
        for (const auto& path : {
                std::filesystem::path("data/assets/backgrounds/menu_background.png"),
                std::filesystem::path("../data/assets/backgrounds/menu_background.png")}) {
            if (std::filesystem::exists(path)) {
                background_ = LoadTexture(path.string().c_str());
                if (background_.id) break;
            }
        }
    }
};

} // namespace openjoey::ui
