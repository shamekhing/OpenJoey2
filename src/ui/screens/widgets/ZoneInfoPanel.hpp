#pragma once
#include "card/Card.hpp"
#include "game/zone/Zone.hpp"
#include "ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

// Right-side panel: zone state, card preview thumbnail, numbered action list.
// All sizes derived from the supplied rect — no fixed pixel values.
struct ZoneInfoPanel {
  static void Draw(Rectangle r, IZone *zone, const char *label,
                   const std::vector<std::string> &actions, int actionCursor,
                   const std::string &lastResult, bool hasSource,
                   CardImageCache &cache, const Texture2D *cardBack) {
    DrawRectangleRec(r, Color{16, 16, 26, 255});
    DrawRectangleLinesEx(r, 1.f, Color{55, 55, 85, 255});

    float pad  = r.width * 0.06f;
    float x    = r.x + pad;
    float maxW = r.width - pad * 2;
    float cy   = r.y + pad;

    int fsTitle = (int)(r.height * 0.032f); fsTitle = std::max(fsTitle, 12);
    int fsSub   = (int)(r.height * 0.024f); fsSub   = std::max(fsSub,   9);
    int fsSmall = (int)(r.height * 0.020f); fsSmall = std::max(fsSmall,  8);
    float lineH = fsSub * 1.5f;

    // ── Zone label
    DrawText(label, (int)x, (int)cy, fsTitle, YELLOW);
    cy += fsTitle + pad * 0.5f;

    // ── Zone type + count
    std::string info = zoneName(zone->type());
    info += "  [" + std::to_string(zone->count()) + "]";
    info += zone->isEmpty() ? "  empty" : "  occupied";
    DrawText(info.c_str(), (int)x, (int)cy, fsSub, COLOR_STAT_TEXT);
    cy += lineH;

    // ── Monster extra state
    if (auto *zm = dynamic_cast<Zone_Monster *>(zone)) {
      std::string ori = zm->position() == Orientation::Vertical ? "ATK" : "DEF";
      std::string vis;
      switch (zm->visibility()) {
      case Visibility::Visible:    vis = "FaceUp (both)";  break;
      case Visibility::Limited:    vis = "FaceUp (owner)"; break;
      case Visibility::Restricted: vis = "FaceDown";       break;
      }
      DrawText(("Pos: " + ori).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY);
      cy += lineH;
      DrawText(("Vis: " + vis).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY);
      cy += lineH;
    }

    // ── Card thumbnail + info
    Card *top = topCard(zone);
    if (top) {
      cy += pad * 0.5f;
      float thumbH = r.height * 0.22f;
      float thumbW = thumbH * (59.f / 86.f);
      Rectangle thumb = {x, cy, thumbW, thumbH};
      const Texture2D *tex = cache.Get(*top);
      bool faceDown = false;
      if (auto *zm = dynamic_cast<Zone_Monster *>(zone))
        faceDown = zm->visibility() == Visibility::Restricted;
      if (!faceDown && tex && tex->id) {
        DrawTexturePro(*tex, {0,0,(float)tex->width,(float)tex->height},
                       thumb, {0,0}, 0.f, WHITE);
      } else if (faceDown) {
        if (cardBack && cardBack->id)
          DrawTexturePro(*cardBack,
                         {0,0,(float)cardBack->width,(float)cardBack->height},
                         thumb, {0,0}, 0.f, WHITE);
        else DrawRectangleRec(thumb, Color{8, 6, 42, 255});
      } else {
        Color fc = top->isMonster() ? Color{88,60,60,255}
                 : top->isSpell()   ? Color{56,90,72,255}
                                    : Color{88,56,92,255};
        DrawRectangleRec(thumb, fc);
      }
      DrawRectangleLinesEx(thumb, 1.2f, Color{200,180,100,255});

      // card text right of thumbnail
      float tx = x + thumbW + pad * 0.6f;
      float tw2 = maxW - thumbW - pad * 0.6f;
      DrawText(clipText(top->name, (int)tw2, fsSub).c_str(),
               (int)tx, (int)cy, fsSub, WHITE);
      cy += fsSub + 3;
      DrawText(top->cardTypeTag().c_str(), (int)tx, (int)cy, fsSmall,
               typeColor(top->type));
      cy += fsSmall + 3;
      if (top->isMonster()) {
        std::string st = "L" + std::to_string(top->level) +
                         "  " + std::to_string(top->atk) +
                         "/" + std::to_string(top->def);
        DrawText(st.c_str(), (int)tx, (int)cy, fsSmall, COLOR_STAT_TEXT);
      }
      cy += thumbH - (fsSub + 3 + fsSmall + 3);
      cy += pad;
    } else {
      cy += pad * 0.5f;
    }

    // ── source indicator
    if (hasSource) {
      DrawRectangleRec({r.x, cy - 2, r.width, fsSub + 6.f},
                       Fade(GREEN, 0.18f));
      DrawText("* SOURCE SELECTED *", (int)x, (int)cy, fsSub, GREEN);
      cy += lineH;
    }

    // ── divider
    DrawLine((int)(r.x + pad * 0.5f), (int)cy,
             (int)(r.x + r.width - pad * 0.5f), (int)cy,
             Color{55, 55, 80, 255});
    cy += pad * 0.5f;

    // ── actions
    DrawText("Actions", (int)x, (int)cy, fsSub, COLOR_STAT_TEXT);
    cy += lineH;
    for (int i = 0; i < (int)actions.size(); ++i) {
      bool sel  = (i == actionCursor);
      Color col = sel ? YELLOW : Color{180, 180, 200, 255};
      if (sel) DrawRectangleRec({r.x, cy - 1, r.width, fsSub + 4.f},
                                Fade(YELLOW, 0.12f));
      std::string line = (sel ? "> " : "  ") +
                         std::to_string(i + 1) + ". " + actions[i];
      DrawText(clipText(line, (int)maxW, fsSmall).c_str(),
               (int)x, (int)cy, fsSmall, col);
      cy += fsSmall * 1.5f;
      if (cy > r.y + r.height - fsSmall * 5) { // leave room for result
        DrawText("...", (int)x, (int)cy, fsSmall, DARKGRAY); break;
      }
    }

    // ── last result (pinned to bottom)
    if (!lastResult.empty()) {
      float ry = r.y + r.height - fsSub * 3.0f;
      DrawLine((int)(r.x + pad * 0.5f), (int)ry,
               (int)(r.x + r.width - pad * 0.5f), (int)ry,
               Color{55, 55, 80, 255});
      ry += pad * 0.3f;
      bool ok = lastResult.find("OK") != std::string::npos ||
                lastResult.find("true") != std::string::npos ||
                lastResult.find("Moved") != std::string::npos;
      DrawText(lastResult.c_str(), (int)x, (int)ry, fsSub,
               ok ? GREEN : Color{220, 80, 80, 255});
    }
  }

private:
  static Card *topCard(IZone *z) {
    if (z->isEmpty()) return nullptr;
    if (auto *zm = dynamic_cast<Zone_Monster *>(z)) return zm->peek();
    if (auto *zs = dynamic_cast<ZoneStack *>(z))    return zs->peek(-1);
    if (auto *zn = dynamic_cast<Zone *>(z))         return zn->peek();
    return nullptr;
  }

  static Color typeColor(CardType t) {
    switch (t) {
    case CardType::Monster: return Color{255, 170, 130, 255};
    case CardType::Spell:   return Color{120, 230, 150, 255};
    case CardType::Trap:    return Color{200, 130, 220, 255};
    }
    return WHITE;
  }

  static std::string zoneName(ZoneType t) {
    switch (t) {
    case ZoneType::Monster:      return "Monster";
    case ZoneType::SpellTrap:    return "Spell/Trap";
    case ZoneType::Field:        return "Field";
    case ZoneType::ExtraMonster: return "Extra Monster";
    case ZoneType::Hand:         return "Hand";
    case ZoneType::Deck:         return "Deck";
    case ZoneType::ExtraDeck:    return "Extra Deck";
    case ZoneType::Graveyard:    return "Graveyard";
    case ZoneType::Banished:     return "Banished";
    case ZoneType::SideDeck:     return "Side Deck";
    }
    return "Unknown";
  }

  static std::string clipText(const std::string &s, int maxPx, int fs) {
    if (MeasureText(s.c_str(), fs) <= maxPx) return s;
    std::string t = s;
    while (t.size() > 1 &&
           MeasureText((t + "..").c_str(), fs) > maxPx)
      t.pop_back();
    return t + "..";
  }
};

} // namespace openjoey::ui
