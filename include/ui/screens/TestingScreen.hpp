#pragma once
#include "ui/AppScreen.hpp"
#include "ui/widgets/StyleSheet.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/screens/IScreen.hpp"
#include <ui/cards/CardPreview.hpp>

#include <raylib.h>
#include <vector>

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

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
        // Empty DB (missing/broken cards.json): stay on the screen and show a
        // hint instead of bouncing away — Draw() handles the empty case too.
        if (!cards_.empty()) {
            if (IsKeyPressed(KEY_RIGHT))
                idx_ = (idx_ + 1) % (int)cards_.size();
            if (IsKeyPressed(KEY_LEFT))
                idx_ = (idx_ - 1 + (int)cards_.size()) % (int)cards_.size();
            preview_.SetCard(&cards_[idx_]);
        }
        return ScreenEvent::none();
    }

    void Draw() const override {
        ClearBackground(COLOR_BG_DARK);
        const int sw = GetScreenWidth();
        const int sh = GetScreenHeight();
        DrawRectangle(0, 0, sw, HEADER_HEIGHT, COLOR_HEADER_BG);
        DrawText("TESTING — image cache", HEADER_TITLE_X, HEADER_TITLE_Y, FONT_SCREEN_TITLE, WHITE);

        // Guard: Draw() can run before the first Update() (screen was just
        // swapped in), so an empty DB must never reach cards_[idx_].
        if (cards_.empty() || idx_ < 0 || idx_ >= (int)cards_.size()) {
            DrawText("No cards loaded — check data/cards.json",
                     HEADER_TITLE_X, HEADER_TITLE_Y + FONT_SCREEN_TITLE + 6,
                     FONT_CARD_STAT, ORANGE);
            return;
        }

        const openjoey::cards::Card& c = cards_[idx_];
        DrawText(TextFormat("%s  #%u  [L/R] next  [ESC] back",
                            c.name.c_str(), c.id),
                 HEADER_TITLE_X, HEADER_TITLE_Y + FONT_SCREEN_TITLE + 6,
                 FONT_CARD_STAT, LIGHTGRAY);

        preview_.Draw({(float)MAIN_PAD_X, (float)(HEADER_HEIGHT + MAIN_PAD_Y),
                       (float)(sw - MAIN_PAD_X * 2),
                       (float)(sh - HEADER_HEIGHT - MAIN_PAD_Y - MAIN_PAD_BOTTOM)},
                      ctx_.imageCache);
    }

private:
    AppContext&                    ctx_;
    std::vector<openjoey::cards::Card>    cards_;
    int                            idx_ = 0;
    mutable CardPreview            preview_;
};

} // namespace openjoey::ui
