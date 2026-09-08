#pragma once
#include <raylib.h>

#include <algorithm>
#include <string>
#include <ui/cards/CardImageCache.hpp>
#include <vector>

#include "cards/Card.hpp"
#include "engine/action/Catalog.hpp"  // which cards have wired effects
#include "ui/widgets/DrawUtils.hpp"
#include "ui/widgets/StyleSheet.hpp"

// Stateful card preview panel (openjoey::ui). Shows portrait image, type/stat
// line, and a scrollable description. Used by DuelScreen and TestingScreen.
// Call SetCard() each frame before Draw() to keep the preview in sync.
namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

class CardPreview {
   public:
    void SetCard(const openjoey::cards::Card* card, bool faceDown = false) {
        if (card != card_) scrollLines_ = 0;  // reset scroll on card change
        card_ = card;
        faceDown_ = faceDown;
    }

    void SetCardBack(const Texture2D* cb) { cardBack_ = cb; }

    void scroll(int delta) {
        scrollLines_ = std::max(0, scrollLines_ - delta);
    }

    // Draw the preview panel into bounds. cache must be the shared AppContext cache.
    void Draw(Rectangle bounds, CardImageCache& cache) const {
        int x = (int)bounds.x, y = (int)bounds.y;
        int w = (int)bounds.width, h = (int)bounds.height;
        int pad = PREVIEW_PAD_X;
        int cy = y + pad;

        DrawRectangle(x, y, w, h, COLOR_BG_MAIN);
        DrawLine(x + w - 1, y, x + w - 1, y + h, COLOR_DIVIDER_LINE);
        DrawText("Preview", x + pad, cy, FONT_PANEL_TITLE, COLOR_STAT_TEXT);
        cy += FONT_PANEL_TITLE + pad / 2;

        // ── Card image
        float aspect = DrawUtils::kCardAspect;
        int cardW = w - pad * 2;
        int cardH = (int)(cardW / aspect);
        if (cardH > h * 48 / 100) {
            cardH = h * 48 / 100;
            cardW = (int)(cardH * aspect);
        }
        Rectangle cardR = {(float)(x + (w - cardW) / 2), (float)cy,
                           (float)cardW, (float)cardH};

        if (faceDown_) {
            if (cardBack_ && cardBack_->id)
                DrawTexturePro(*cardBack_,
                               {0, 0, (float)cardBack_->width, (float)cardBack_->height},
                               cardR, {0, 0}, 0.f, WHITE);
            else
                DrawRectangleRec(cardR, COLOR_CARD_BACK_FG);
            DrawRectangleLinesEx(cardR, 1.5f, Color{210, 170, 40, 255});
        } else if (card_) {
            const Texture2D* tex = cache.Get(*card_);
            if (tex && tex->id) {
                DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                               cardR, {0, 0}, 0.f, WHITE);
            } else {
                Color fc = card_->isMonster() ? COLOR_MONSTER_STAT
                           : card_->isSpell() ? COLOR_SPELL_STAT
                                              : COLOR_TRAP_STAT;
                DrawRectangleRec(cardR, Fade(fc, 0.4f));
            }
            DrawRectangleLinesEx(cardR, 1.2f, Color{200, 180, 100, 255});
        } else {
            DrawRectangleRec(cardR, COLOR_BG_MAIN);
            DrawRectangleLinesEx(cardR, 1.f, COLOR_DIVIDER_LINE);
        }
        cy += cardH + pad;

        if (!card_ || faceDown_) return;

        // ── Card text
        DrawText(card_->name.c_str(), x + pad, cy, FONT_CARD_NAME, WHITE);
        cy += FONT_CARD_NAME + 3;
        DrawText(card_->cardTypeTag().c_str(), x + pad, cy, FONT_CARD_STAT,
                 card_->isMonster() ? COLOR_MONSTER_STAT
                 : card_->isSpell() ? COLOR_SPELL_STAT
                                    : COLOR_TRAP_STAT);
        cy += FONT_CARD_STAT + 3;
        // Honesty marker: a Spell/Trap with no wired classic effect can only be
        // SET, never activated — say so instead of silently ignoring its text.
        if ((card_->isSpell() || card_->isTrap()) &&
            !openjoey::engine::action::findClassicEffect(card_->name)) {
            DrawText("[effect not wired in this build]", x + pad, cy, FONT_CARD_STAT,
                     Color{230, 170, 60, 255});
            cy += FONT_CARD_STAT + 3;
        }
        if (card_->isMonster()) {
            DrawText(card_->statLine().c_str(), x + pad, cy, FONT_CARD_STAT, COLOR_STAT_TEXT);
            cy += FONT_CARD_STAT + 4;
        }

        // ── Scrollable description (cached: wrapText is O(n^2) in MeasureText
        // calls — too costly to run every frame)
        int lineFs = FONT_HELP_TEXT;
        int lineH = lineFs + 3;
        int maxPx = w - pad * 2;
        if (card_ != wrappedFor_ || maxPx != wrappedWidth_) {
            wrappedLines_ = DrawUtils::wrapText(card_->description, maxPx, lineFs);
            wrappedFor_ = card_;
            wrappedWidth_ = maxPx;
        }
        const auto& lines = wrappedLines_;

        int maxScroll = std::max(0, (int)lines.size() - 1);
        int scroll = std::min(scrollLines_, maxScroll);

        for (int i = scroll; i < (int)lines.size(); ++i) {
            if (cy + lineH > y + h - pad) break;
            DrawText(lines[i].c_str(), x + pad, cy, lineFs, COLOR_DESC_TEXT);
            cy += lineH;
        }

        if ((int)lines.size() > 1) {
            int barX = x + w - pad / 2 - 2;
            int barTop = y + cardH + pad * 3;
            int barH = y + h - pad - barTop;
            DrawRectangle(barX, barTop, 2, barH, COLOR_SCROLLBAR_BG);
            int thumbH = std::max(barH / (int)lines.size(), int(0.01f * h));
            int thumbY = barTop + (barH - thumbH) * scroll / std::max(maxScroll, 1);
            DrawRectangle(barX, thumbY, 2, thumbH, COLOR_SCROLLBAR_THUMB);
        }
    }

   private:
    const openjoey::cards::Card* card_ = nullptr;
    bool faceDown_ = false;
    const Texture2D* cardBack_ = nullptr;
    int scrollLines_ = 0;

    // Description wrap cache (Draw() is const; the UI runs single-threaded).
    mutable const openjoey::cards::Card* wrappedFor_ = nullptr;
    mutable int wrappedWidth_ = -1;
    mutable std::vector<std::string> wrappedLines_;
};

}  // namespace openjoey::ui
