#pragma once
#include "card/Card.hpp"
#include "game/zone/Zone.hpp"
#include "ui/core/Theme.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include "ui/renderer/DrawUtils.hpp"
#include <algorithm>
#include <raylib.h>
#include <string>

namespace openjoey::ui {
using namespace openjoey::zone;

// Draws a single zone slot (card face, border, label). All sizes derived
// from the caller rect. Visual constants come exclusively from Theme.
struct ZoneCell {
    static void Draw(Rectangle r, IZone* zone, const char* label,
                     bool isCursor, bool isSelected,
                     CardImageCache& cache, const Texture2D* cardBack,
                     const Theme& t) {
        DrawRectangleRec(r, zoneBg(zone->type(), t));

        float thick  = isCursor     ? r.height * 0.022f
                       : isSelected ? r.height * 0.018f : 1.0f;
        Color border = isCursor     ? t.colors.cursorBorder
                       : isSelected ? t.colors.selectedBorder
                                    : t.colors.zoneCellBorder;
        DrawRectangleLinesEx(r, thick, border);
        if (isCursor)
            DrawRectangleLinesEx({r.x - 2, r.y - 2, r.width + 4, r.height + 4},
                                 1.f, Fade(t.colors.cursorBorder, 0.3f));

        float pad  = r.width * 0.05f;
        float labH = r.height * 0.14f;
        Rectangle inner = {r.x + pad, r.y + pad,
                           r.width - pad * 2, r.height - pad * 2 - labH};

        if (auto* zm = dynamic_cast<Zone_Monster*>(zone))
            drawMonster(inner, zm, cache, cardBack, t);
        else if (auto* z = dynamic_cast<Zone*>(zone))
            drawSingle(inner, z, cache, t);
        else if (auto* zs = dynamic_cast<ZoneStack*>(zone))
            drawStack(inner, zs, cache, cardBack, t);

        int fs = std::max(8, (int)(r.height * 0.12f));
        int tw = MeasureText(label, fs);
        DrawText(label, (int)(r.x + (r.width - tw) * 0.5f),
                 (int)(r.y + r.height - labH * 0.85f), fs,
                 Color{150, 150, 180, 200});
    }

private:
    static Color zoneBg(ZoneType type, const Theme& t) {
        switch (type) {
        case ZoneType::Monster:      return t.colors.zoneBgMonster;
        case ZoneType::SpellTrap:    return t.colors.zoneBgSpellTrap;
        case ZoneType::Field:        return t.colors.zoneBgField;
        case ZoneType::ExtraMonster: return t.colors.zoneBgExtraMonster;
        case ZoneType::Deck:         return t.colors.zoneBgDeck;
        case ZoneType::ExtraDeck:    return t.colors.zoneBgExtraDeck;
        case ZoneType::Graveyard:    return t.colors.zoneBgGraveyard;
        case ZoneType::Banished:     return t.colors.zoneBgBanished;
        default:                     return t.colors.zoneBgDefault;
        }
    }

    static Color cardFaceColor(CardType type, const Theme& t) {
        switch (type) {
        case CardType::Monster: return t.colors.cardFaceMonster;
        case CardType::Spell:   return t.colors.cardFaceSpell;
        case CardType::Trap:    return t.colors.cardFaceTrap;
        }
        return t.colors.cardFaceUnknown;
    }

    static void drawCardBack(Rectangle dst, const Texture2D* cb, bool rotateDef,
                             const Theme& t) {
        if (cb && cb->id) {
            DrawUtils::blitCard(dst, *cb, rotateDef);
            DrawRectangleLinesEx(dst, 1.5f, t.colors.cardBorderFaceDown);
            return;
        }
        DrawRectangleRec(dst, t.colors.cardBackFg);
        float cx = dst.x + dst.width * 0.5f, cy = dst.y + dst.height * 0.5f;
        float dw = dst.width * 0.42f,  dh = dst.height * 0.35f;
        DrawLineEx({cx, cy - dh}, {cx + dw, cy},      1.5f, t.colors.cardBorderFaceDown);
        DrawLineEx({cx + dw, cy}, {cx, cy + dh},      1.5f, t.colors.cardBorderFaceDown);
        DrawLineEx({cx, cy + dh}, {cx - dw, cy},      1.5f, t.colors.cardBorderFaceDown);
        DrawLineEx({cx - dw, cy}, {cx, cy - dh},      1.5f, t.colors.cardBorderFaceDown);
        DrawRectangleLinesEx(dst, 1.5f, t.colors.cardBorderFaceDown);
    }

    static void drawFallback(Rectangle dst, Card* c, bool faceDown,
                             const Texture2D* cb, bool rotateDef, const Theme& t) {
        if (faceDown) { drawCardBack(dst, cb, rotateDef, t); return; }
        if (!c) return;
        DrawRectangleRec(dst, cardFaceColor(c->type, t));
        DrawRectangleLinesEx(dst, 1.2f, Fade(WHITE, 0.35f));
        int fs = std::max(8, (int)(dst.height * 0.13f));
        std::string nm = c->name.size() > 9 ? c->name.substr(0, 8) + "." : c->name;
        DrawText(nm.c_str(), (int)dst.x + 3, (int)dst.y + 3, fs, WHITE);
        if (c->isMonster()) {
            std::string st = std::to_string(c->atk) + "/" + std::to_string(c->def);
            DrawText(st.c_str(), (int)dst.x + 3, (int)(dst.y + dst.height - fs - 3),
                     fs, Fade(WHITE, 0.8f));
        }
    }

    static void drawMonster(Rectangle inner, Zone_Monster* zm,
                            CardImageCache& cache, const Texture2D* cb,
                            const Theme& t) {
        if (zm->isEmpty()) return;
        Card* c   = zm->peek();
        bool  atk = zm->position() == Orientation::Vertical;
        bool  fd  = zm->visibility() == Visibility::Restricted;
        bool drew = false;
        if (!fd && c) {
            const Texture2D* tex = cache.Get(*c);
            if (tex && tex->id) { DrawUtils::blitCard(inner, *tex, !atk); drew = true; }
        }
        if (!drew) drawFallback(inner, c, fd, cb, !atk, t);
        if (!fd && c) {
            int   fs = std::max(8, (int)(inner.height * 0.12f));
            Color bc = atk ? t.colors.atkBadge : t.colors.defBadge;
            DrawRectangle((int)inner.x + 2, (int)inner.y + 2, fs + 4, fs + 2,
                          Fade(BLACK, 0.65f));
            DrawText(atk ? "A" : "D", (int)inner.x + 4, (int)inner.y + 3, fs, bc);
        }
    }

    static void drawSingle(Rectangle inner, Zone* z, CardImageCache& cache,
                           const Theme& t) {
        if (z->isEmpty()) return;
        Card* c = z->peek();
        float cw = inner.width * 0.75f, ch = inner.height * 0.95f;
        Rectangle cr = {inner.x + (inner.width - cw) * 0.5f,
                        inner.y + (inner.height - ch) * 0.5f, cw, ch};
        const Texture2D* tex = cache.Get(*c);
        if (tex && tex->id) { DrawUtils::blitCard(cr, *tex, false); return; }
        drawFallback(cr, c, false, nullptr, false, t);
    }

    static void drawStack(Rectangle inner, ZoneStack* zs,
                          CardImageCache& cache, const Texture2D* cb,
                          const Theme& t) {
        if (zs->isEmpty()) return;
        int   layers = std::min(zs->count(), 4);
        float off    = inner.width * 0.02f;
        for (int i = layers - 1; i >= 1; --i)
            DrawRectangleRec({inner.x + i * off, inner.y + i * off,
                              inner.width - i * off, inner.height - i * off},
                             Color{40, 40, 60, 160});
        bool  useBack = zs->type() == ZoneType::Deck || zs->type() == ZoneType::ExtraDeck;
        Card* top     = zs->peek(-1);
        if (useBack) {
            drawCardBack(inner, cb, false, t);
        } else if (top) {
            const Texture2D* tex = cache.Get(*top);
            if (tex && tex->id) DrawUtils::blitCard(inner, *tex, false);
            else                drawFallback(inner, top, false, nullptr, false, t);
        }
        int fs = std::max(8, (int)(inner.width * 0.18f));
        int bw = fs + 8;
        DrawRectangle((int)(inner.x + inner.width - bw), (int)inner.y, bw, fs + 4,
                      Fade(BLACK, 0.75f));
        DrawText(std::to_string(zs->count()).c_str(),
                 (int)(inner.x + inner.width - bw + 3), (int)inner.y + 2, fs, YELLOW);
    }
};

} // namespace openjoey::ui
