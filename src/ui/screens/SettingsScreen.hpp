#pragma once
#include "ui/AppScreen.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/platform/Settings.hpp"
#include "ui/screens/IScreen.hpp"
#include "ui/widgets/input/KeyboardNav.hpp"

#include <raylib.h>
#include <string>

namespace openjoey::ui {

// Toggles fullscreen / target FPS / resolution / image-download preference.
// Changes are applied live to the running raylib window and persisted to
// data/user_settings.json on every edit.
class SettingsScreen : public IScreen {
public:
    explicit SettingsScreen(AppContext& ctx) : ctx_(ctx) { nav_.setCount(kOptCount); }

    ScreenEvent Update(float) override {
        nav_.handleWrapKeys();

        if (IsKeyPressed(KEY_ENTER)) {
            apply(nav_.cursor);
            ctx_.settings.Save();
            statusMsg_ = "Saved";
        }
        if (IsKeyPressed(KEY_ESCAPE))
            return ScreenEvent::replace(AppScreen::MainMenu);
        return ScreenEvent::none();
    }

    void Draw() const override {
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        ClearBackground(COLOR_BG_DARK);

        DrawRectangle(0, 0, sw, HEADER_HEIGHT, COLOR_HEADER_BG);
        DrawText("SETTINGS", HEADER_TITLE_X, HEADER_TITLE_Y, FONT_SCREEN_TITLE, WHITE);
        DrawText("[ESC] back   [UP/DN] choose   [ENTER] toggle",
                 sw - HELP_TEXT_X_OFFSET, HELP_TEXT_Y, FONT_CARD_TYPE, GRAY);

        const Settings& s = ctx_.settings;
        const char* labels[kOptCount] = {
            "Fullscreen", "Target FPS", "Resolution", "Download card images"
        };
        std::string values[kOptCount] = {
            s.fullscreen ? "ON" : "OFF",
            std::to_string(s.targetFps) + " FPS",
            std::to_string(s.screenWidth) + "x" + std::to_string(s.screenHeight),
            s.downloadImages ? "ON" : "OFF",
        };
        const int baseY = sh / 2 - kOptCount * MENU_ITEM_SPACING / 2;
        for (int i = 0; i < kOptCount; ++i) {
            Color c = (i == nav_.cursor) ? YELLOW : LIGHTGRAY;
            DrawText(labels[i], sw / 2 - 220, baseY + i * MENU_ITEM_SPACING, FONT_MENU_ITEM, c);
            DrawText(values[i].c_str(), sw / 2 + 40, baseY + i * MENU_ITEM_SPACING, FONT_MENU_ITEM, c);
        }
        if (!statusMsg_.empty())
            DrawText(TextFormat("  %s", statusMsg_.c_str()), HEADER_TITLE_X,
                     sh - MENU_HELP_BOTTOM_OFFSET, FONT_HELP_SMALL, GREEN);
    }

private:
    static constexpr int kOptCount   = 4;
    static constexpr int kWidths[]   = {1280, 1620, 1920};
    static constexpr int kHeights[]  = {720, 920, 1080};
    static constexpr int kFps[]      = {30, 60, 120};

    AppContext&  ctx_;
    KeyboardNav  nav_;
    std::string  statusMsg_;

    void apply(int which) {
        Settings& s = ctx_.settings;
        switch (which) {
            case 0:
                s.fullscreen = !s.fullscreen;
                applyWindow(s);
                break;
            case 1: {
                int idx = 0;
                for (int i = 0; i < 3; ++i) if (kFps[i] == s.targetFps) idx = i;
                s.targetFps = kFps[(idx + 1) % 3];
                SetTargetFPS(s.targetFps);
                break;
            }
            case 2: {
                int wi = 0, hi = 0;
                for (int i = 0; i < 3; ++i) {
                    if (kWidths[i]  == s.screenWidth)  wi = i;
                    if (kHeights[i] == s.screenHeight) hi = i;
                }
                s.screenWidth  = kWidths[(wi + 1) % 3];
                s.screenHeight = kHeights[(hi + 1) % 3];
                applyWindow(s);
                break;
            }
            case 3:
                s.downloadImages = !s.downloadImages;
                break;
        }
    }

    static void applyWindow(const Settings& s) {
        if (s.fullscreen) {
            SetWindowState(FLAG_FULLSCREEN_MODE);
        } else {
            ClearWindowState(FLAG_FULLSCREEN_MODE);
            SetWindowSize(s.screenWidth, s.screenHeight);
        }
    }
};

} // namespace openjoey::ui
