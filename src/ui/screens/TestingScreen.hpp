#pragma once
#include "ui/AppScreen.hpp"
#include "ui/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include "card/ui/CardPreview.hpp"

#include <raylib.h>
#include <vector>

namespace openjoey::ui {

// Diagnostic screen: cycles the loaded card database and renders each card via
// the shared CardPreview widget. Useful for verifying CardImageCache download +
// cache behaviour at runtime.
class TestingScreen : public IScreen {
public:
    explicit TestingScreen(AppContext& ctx) : ctx_(ctx) {
        const auto& src = ctx_.cardDb.GetAllCards();
        cards_.assign(src.begin(), src.end());
    }

    ScreenEvent Update(float) override {
        ctx_.imageCache.PollAndLoad();

        if (IsKeyPressed(KEY_ESCAPE))
            return ScreenEvent::replace(AppScreen::MainMenu);
        if (cards_.empty())
            return ScreenEvent::replace(AppScreen::MainMenu);
        if (IsKeyPressed(KEY_RIGHT))
            idx_ = (idx_ + 1) % (int)cards_.size();
        if (IsKeyPressed(KEY_LEFT))
            idx_ = (idx_ - 1 + (int)cards_.size()) % (int)cards_.size();

        preview_.SetCard(&cards_[idx_]);
        return ScreenEvent::none();
    }

    void Draw() const override {
        ClearBackground(COLOR_BG_DARK);
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        DrawRectangle(0, 0, sw, HEADER_HEIGHT, COLOR_HEADER_BG);

        const openjoey::Card& c = cards_[idx_];
        DrawText("TESTING — image cache", HEADER_TITLE_X, HEADER_TITLE_Y, FONT_SCREEN_TITLE, WHITE);
        DrawText(TextFormat("%s  #%u  [L/R] next  [ESC] back",
                            c.name.c_str(), c.cardId),
                 HEADER_TITLE_X, HEADER_TITLE_Y + FONT_SCREEN_TITLE + 6,
                 FONT_CARD_STAT, LIGHTGRAY);

        preview_.Draw({(float)MAIN_PAD_X, (float)(HEADER_HEIGHT + MAIN_PAD_Y),
                       (float)(sw - MAIN_PAD_X * 2),
                       (float)(sh - HEADER_HEIGHT - MAIN_PAD_Y - MAIN_PAD_BOTTOM)},
                      ctx_.imageCache);
    }

private:
    AppContext&                    ctx_;
    std::vector<openjoey::Card>    cards_;
    int                            idx_ = 0;
    mutable CardPreview            preview_;
};

} // namespace openjoey::ui
