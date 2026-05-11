#pragma once
#include "card/Card.hpp"
#include "game/zone/Zone.hpp"
#include "ui/core/Theme.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include "ui/renderer/DrawUtils.hpp"
#include <algorithm>
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

class CardPreview {
public:
    void SetCard(const openjoey::Card* card, bool faceDown = false) {
        if (card != card_) scrollLines_ = 0;
        card_     = card;
        faceDown_ = faceDown;
    }

    void SetCardBack(const Texture2D* cb) { cardBack_ = cb; }

    void scroll(int delta) {
        scrollLines_ = std::max(0, scrollLines_ - delta);
    }

    void Draw(Rectangle bounds, CardImageCache& cache) const {
        const Theme t = Theme::FromScreen();
        int x   = (int)bounds.x, y = (int)bounds.y;
        int w   = (int)bounds.width, h = (int)bounds.height;
        int pad = t.previewPadX;
        int cy  = y + pad;

        DrawRectangle(x, y, w, h, t.colors.bgMain);
        DrawLine(x + w - 1, y, x + w - 1, y + h, t.colors.dividerLine);
        DrawText("Preview", x + pad, cy, t.fontPanelTitle, t.colors.statText);
        cy += t.fontPanelTitle + pad / 2;

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
                DrawRectangleRec(cardR, t.colors.cardBackFg);
            DrawRectangleLinesEx(cardR, 1.5f, t.colors.cardBorderFaceDown);
        } else if (card_) {
            const Texture2D* tex = cache.Get(*card_);
            if (tex && tex->id) {
                DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                               cardR, {0, 0}, 0.f, WHITE);
            } else {
                Color fc = card_->isMonster() ? t.colors.monsterStat
                           : card_->isSpell() ? t.colors.spellStat
                                              : t.colors.trapStat;
                DrawRectangleRec(cardR, Fade(fc, 0.4f));
            }
            DrawRectangleLinesEx(cardR, 1.2f, t.colors.cardBorderFaceUp);
        } else {
            DrawRectangleRec(cardR, t.colors.bgMain);
            DrawRectangleLinesEx(cardR, 1.f, t.colors.dividerLine);
        }
        cy += cardH + pad;

        if (!card_ || faceDown_) return;

        DrawText(card_->name.c_str(), x + pad, cy, t.fontCardName, WHITE);
        cy += t.fontCardName + 3;
        DrawText(card_->cardTypeTag().c_str(), x + pad, cy, t.fontCardStat,
                 card_->isMonster() ? t.colors.monsterStat
                 : card_->isSpell() ? t.colors.spellStat
                                    : t.colors.trapStat);
        cy += t.fontCardStat + 3;
        if (card_->isMonster()) {
            DrawText(card_->statLine().c_str(), x + pad, cy, t.fontCardStat, t.colors.statText);
            cy += t.fontCardStat + 4;
        }

        int lineFs = t.fontHelpText;
        int lineH  = lineFs + 3;
        int maxPx  = w - pad * 2;
        auto lines = DrawUtils::wrapText(card_->description, maxPx, lineFs);

        int maxScroll = std::max(0, (int)lines.size() - 1);
        int scroll    = std::min(scrollLines_, maxScroll);

        for (int i = scroll; i < (int)lines.size(); ++i) {
            if (cy + lineH > y + h - pad) break;
            DrawText(lines[i].c_str(), x + pad, cy, lineFs, t.colors.descText);
            cy += lineH;
        }

        if ((int)lines.size() > 1) {
            int barX   = x + w - pad / 2 - 2;
            int barTop = y + cardH + pad * 3;
            int barH   = y + h - pad - barTop;
            DrawRectangle(barX, barTop, 2, barH, t.colors.scrollbarBg);
            int thumbH = std::max(barH / (int)lines.size(), int(0.01f * h));
            int thumbY = barTop + (barH - thumbH) * scroll / std::max(maxScroll, 1);
            DrawRectangle(barX, thumbY, 2, thumbH, t.colors.scrollbarThumb);
        }
    }

private:
    const openjoey::Card* card_        = nullptr;
    bool                  faceDown_    = false;
    const Texture2D*      cardBack_    = nullptr;
    int                   scrollLines_ = 0;
};

} // namespace openjoey::ui
