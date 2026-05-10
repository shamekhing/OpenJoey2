#pragma once
#include "card/Card.hpp"
#include "game/zone/Zone.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include <raylib.h>
#include <string>

namespace openjoey::ui {
using namespace openjoey::zone;

// Draws a single zone slot (card face, border, label). All sizes are derived
// from the caller-supplied rect — no fixed pixel values.
struct ZoneCell {
  static void Draw(Rectangle r, IZone *zone, const char *label, bool isCursor,
                   bool isSelected, CardImageCache &cache,
                   const Texture2D *cardBack) {
    // ── zone background
    DrawRectangleRec(r, zoneBg(zone->type()));

    // ── border / glow
    float thick = isCursor     ? r.height * 0.022f
                  : isSelected ? r.height * 0.018f
                               : 1.0f;
    Color border = isCursor     ? Color{255, 220, 0, 255}
                   : isSelected ? Color{60, 220, 80, 255}
                                : Color{55, 55, 80, 140};
    DrawRectangleLinesEx(r, thick, border);
    if (isCursor)
      DrawRectangleLinesEx({r.x - 2, r.y - 2, r.width + 4, r.height + 4}, 1.f,
                           Fade(YELLOW, 0.3f));

    // ── card content
    float pad = r.width * 0.05f;
    float labH = r.height * 0.14f;
    Rectangle inner = {r.x + pad, r.y + pad, r.width - pad * 2,
                       r.height - pad * 2 - labH};

    if (auto *zm = dynamic_cast<Zone_Monster *>(zone))
      drawMonster(inner, zm, cache, cardBack);
    else if (auto *z = dynamic_cast<Zone *>(zone))
      drawSingle(inner, z, cache);
    else if (auto *zs = dynamic_cast<ZoneStack *>(zone))
      drawStack(inner, zs, cache, cardBack);

    // ── label
    int fs = (int)(r.height * 0.12f);
    fs = std::max(fs, 8);
    int tw = MeasureText(label, fs);
    DrawText(label, (int)(r.x + (r.width - tw) * 0.5f),
             (int)(r.y + r.height - labH * 0.85f), fs,
             Color{150, 150, 180, 200});
  }

private:
  static constexpr float kAspect = 59.f / 86.f; // card W:H

  static Color zoneBg(ZoneType t) {
    switch (t) {
    case ZoneType::Monster:
      return {50, 14, 14, 220};
    case ZoneType::SpellTrap:
      return {12, 48, 34, 220};
    case ZoneType::Field:
      return {12, 30, 58, 220};
    case ZoneType::ExtraMonster:
      return {42, 12, 58, 220};
    case ZoneType::Deck:
      return {22, 22, 32, 220};
    case ZoneType::ExtraDeck:
      return {28, 14, 48, 220};
    case ZoneType::Graveyard:
      return {48, 22, 8, 220};
    case ZoneType::Banished:
      return {50, 38, 8, 220};
    default:
      return {20, 20, 30, 220};
    }
  }

  static Color cardTypeColor(CardType t) {
    switch (t) {
    case CardType::Monster:
      return {88, 60, 60, 255};
    case CardType::Spell:
      return {56, 90, 72, 255};
    case CardType::Trap:
      return {88, 56, 92, 255};
    }
    return {60, 60, 60, 255};
  }

  // Draw card texture fitted into rect, optionally rotated 90° for DEF.
  static void blitCard(Rectangle dst, const Texture2D *tex, bool rotateDef) {
    Rectangle src = {0, 0, (float)tex->width, (float)tex->height};
    if (!rotateDef) {
      // fit portrait inside dst, preserve aspect
      float rectAsp = dst.width / dst.height;
      Rectangle fit = dst;
      if (rectAsp > kAspect) {
        fit.width = dst.height * kAspect;
        fit.x = dst.x + (dst.width - fit.width) * 0.5f;
      } else {
        fit.height = dst.width / kAspect;
        fit.y = dst.y + (dst.height - fit.height) * 0.5f;
      }
      DrawTexturePro(*tex, src, fit, {0, 0}, 0.f, WHITE);
    } else {
      // Portrait texture rotated 90° CW → appears landscape on screen.
      // Post-rotation aspect ratio is 86:59 (1/kAspect). Fit-within dst:
      float cx = dst.x + dst.width * 0.5f;
      float cy = dst.y + dst.height * 0.5f;
      float postAspect = 1.f / kAspect; // landscape: 86/59
      float postW, postH;
      if (dst.width / dst.height >= postAspect) {
        postH = dst.height;
        postW = dst.height * postAspect;
      } else {
        postW = dst.width;
        postH = dst.width / postAspect;
      }
      // After 90° CW: pre_width → screen height, pre_height → screen width
      float preW = postH;
      float preH = postW;
      Rectangle d = {cx - preW * 0.5f, cy - preH * 0.5f, preW, preH};
      DrawTexturePro(*tex, src, d, {preW * 0.5f, preH * 0.5f}, 90.f, WHITE);
    }
  }

  static void drawCardBack(Rectangle dst, const Texture2D *cardBack,
                           bool rotateDef) {
    if (cardBack && cardBack->id) {
      blitCard(dst, cardBack, rotateDef);
      DrawRectangleLinesEx(dst, 1.5f, Color{210, 170, 40, 255});
      return;
    }
    // Procedural fallback: navy + gold diamond
    const Color kNavy = {8, 6, 42, 255};
    const Color kGold = {210, 170, 40, 255};
    DrawRectangleRec(dst, kNavy);
    float cx = dst.x + dst.width * 0.5f, cy = dst.y + dst.height * 0.5f;
    float dw = dst.width * 0.42f, dh = dst.height * 0.35f;
    DrawLineEx({cx, cy - dh}, {cx + dw, cy}, 1.5f, kGold);
    DrawLineEx({cx + dw, cy}, {cx, cy + dh}, 1.5f, kGold);
    DrawLineEx({cx, cy + dh}, {cx - dw, cy}, 1.5f, kGold);
    DrawLineEx({cx - dw, cy}, {cx, cy - dh}, 1.5f, kGold);
    DrawRectangleLinesEx(dst, 1.5f, kGold);
  }

  static void drawFallback(Rectangle dst, Card *c, bool faceDown,
                           const Texture2D *cardBack, bool rotateDef) {
    if (faceDown) {
      drawCardBack(dst, cardBack, rotateDef);
      return;
    }
    if (!c)
      return;
    Color fill = cardTypeColor(c->type);
    DrawRectangleRec(dst, fill);
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

  static void drawMonster(Rectangle inner, Zone_Monster *zm,
                          CardImageCache &cache, const Texture2D *cardBack) {
    if (zm->isEmpty())
      return;
    Card *c = zm->peek();
    bool atk = zm->position() == Orientation::Vertical;
    bool faceDown = zm->visibility() == Visibility::Restricted;

    // card rect: portrait for ATK, landscape area for DEF
    float cw = atk ? inner.width * 0.70f : inner.width * 0.95f;
    float ch = atk ? inner.height * 0.95f : inner.height * 0.65f;
    Rectangle cr = {inner.x + (inner.width - cw) * 0.5f,
                    inner.y + (inner.height - ch) * 0.5f, cw, ch};

    if (!faceDown && c) {
      const Texture2D *tex = cache.Get(*c);
      if (tex && tex->id) {
        blitCard(cr, tex, !atk);
        goto badge;
      }
    }
    drawFallback(cr, c, faceDown, cardBack, !atk);

  badge:
    if (!faceDown && c) {
      // ATK/DEF badge
      int fs = std::max(8, (int)(inner.height * 0.12f));
      const char *badge = atk ? "A" : "D";
      Color bc = atk ? Color{150, 255, 150, 255} : Color{100, 200, 255, 255};
      DrawRectangle((int)inner.x + 2, (int)inner.y + 2, fs + 4, fs + 2,
                    Fade(BLACK, 0.65f));
      DrawText(badge, (int)inner.x + 4, (int)inner.y + 3, fs, bc);
    }
  }

  static void drawSingle(Rectangle inner, Zone *z, CardImageCache &cache) {
    if (z->isEmpty())
      return;
    Card *c = z->peek();
    float cw = inner.width * 0.75f;
    float ch = inner.height * 0.95f;
    Rectangle cr = {inner.x + (inner.width - cw) * 0.5f,
                    inner.y + (inner.height - ch) * 0.5f, cw, ch};
    const Texture2D *tex = cache.Get(*c);
    if (tex && tex->id) {
      blitCard(cr, tex, false);
      return;
    }
    drawFallback(cr, c, false, nullptr, false);
  }

  static void drawStack(Rectangle inner, ZoneStack *zs, CardImageCache &cache,
                        const Texture2D *cardBack) {
    if (zs->isEmpty())
      return;
    // shadow stack
    int layers = std::min(zs->count(), 4);
    float off = inner.width * 0.02f;
    for (int i = layers - 1; i >= 1; --i)
      DrawRectangleRec({inner.x + i * off, inner.y + i * off,
                        inner.width - i * off, inner.height - i * off},
                       Color{40, 40, 60, 160});

    bool useBack =
        zs->type() == ZoneType::Deck || zs->type() == ZoneType::ExtraDeck;
    Card *top = zs->peek(-1);
    if (useBack) {
      drawCardBack(inner, cardBack, false);
    } else if (top) {
      const Texture2D *tex = cache.Get(*top);
      if (tex && tex->id)
        blitCard(inner, tex, false);
      else
        drawFallback(inner, top, false, nullptr, false);
    }

    // count badge (top-right corner)
    int fs = std::max(8, (int)(inner.width * 0.18f));
    int bw = fs + 8;
    DrawRectangle((int)(inner.x + inner.width - bw), (int)inner.y, bw, fs + 4,
                  Fade(BLACK, 0.75f));
    std::string cnt = std::to_string(zs->count());
    DrawText(cnt.c_str(), (int)(inner.x + inner.width - bw + 3),
             (int)inner.y + 2, fs, YELLOW);
  }
};

} // namespace openjoey::ui
