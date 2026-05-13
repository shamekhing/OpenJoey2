#pragma once
#include "zone/Zone.hpp"
#include "ui/core/Theme.hpp"
#include "ui/renderer/DrawUtils.hpp"
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {
using namespace openjoey::zone;

// Draws zone info + action list inside any given rectangle.
// Designed to work both as a permanent panel and as popup content.
struct ZoneInfoPanel {
    static void Draw(Rectangle r, IZone* zone, const char* label,
                     const std::vector<std::string>& actions, int actionCursor,
                     const std::string& lastResult, bool hasSource) {
        if (!zone) return;

        const Theme t = Theme::FromScreen();
        DrawRectangleRec(r, t.colors.panelBg);
        DrawRectangleLinesEx(r, 1.f, t.colors.panelBorder);

        float pad  = r.width * 0.06f;
        float x    = r.x + pad;
        float maxW = r.width - pad * 2;
        float cy   = r.y + pad;

        int fsTitle = std::max(12, (int)(r.height * 0.032f));
        int fsSub   = std::max(9,  (int)(r.height * 0.024f));
        int fsSmall = std::max(8,  (int)(r.height * 0.020f));
        float lineH = fsSub * 1.5f;

        DrawText(label, (int)x, (int)cy, fsTitle, YELLOW);
        cy += fsTitle + pad * 0.5f;

        std::string info  = zoneName(zone->type());
        info += "  [" + std::to_string(zone->count()) + "]";
        info += zone->isEmpty() ? "  empty" : "  occupied";
        DrawText(info.c_str(), (int)x, (int)cy, fsSub, t.colors.statText);
        cy += lineH;

        if (auto* zm = dynamic_cast<Zone_Monster*>(zone)) {
            std::string ori = zm->position() == Orientation::Vertical ? "ATK" : "DEF";
            std::string vis;
            switch (zm->visibility()) {
            case Visibility::Visible:    vis = "FaceUp (both)";  break;
            case Visibility::Limited:    vis = "FaceUp (owner)"; break;
            case Visibility::Restricted: vis = "FaceDown";       break;
            }
            DrawText(("Pos: " + ori).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY); cy += lineH;
            DrawText(("Vis: " + vis).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY); cy += lineH;
        }
        cy += pad * 0.5f;

        if (hasSource) {
            DrawRectangleRec({r.x, cy - 2, r.width, fsSub + 6.f},
                             Fade(t.colors.selectedBorder, 0.18f));
            DrawText("* SOURCE SELECTED *", (int)x, (int)cy, fsSub, t.colors.selectedBorder);
            cy += lineH;
        }

        DrawLine((int)(r.x + pad * 0.5f), (int)cy,
                 (int)(r.x + r.width - pad * 0.5f), (int)cy, t.colors.dividerLine);
        cy += pad * 0.5f;

        DrawText("Actions  [Tab] cycle  [Space] exec  [#][Enter] by index",
                 (int)x, (int)cy, fsSmall, t.colors.statText);
        cy += lineH;

        for (int i = 0; i < (int)actions.size(); ++i) {
            bool  sel = (i == actionCursor);
            Color col = sel ? t.colors.cursorBorder : t.colors.statText;
            if (sel)
                DrawRectangleRec({r.x, cy - 1, r.width, fsSub + 4.f},
                                 Fade(t.colors.cursorBorder, 0.12f));
            std::string line = (sel ? "> " : "  ") + std::to_string(i + 1) + ". " + actions[i];
            DrawText(DrawUtils::clipText(line, (int)maxW, fsSmall).c_str(),
                     (int)x, (int)cy, fsSmall, col);
            cy += fsSmall * 1.5f;
            if (cy > r.y + r.height - fsSmall * 5) {
                DrawText("...", (int)x, (int)cy, fsSmall, DARKGRAY);
                break;
            }
        }

        if (!lastResult.empty()) {
            float ry = r.y + r.height - fsSub * 3.0f;
            DrawLine((int)(r.x + pad * 0.5f), (int)ry,
                     (int)(r.x + r.width - pad * 0.5f), (int)ry, t.colors.dividerLine);
            ry += pad * 0.3f;
            bool ok = lastResult.find("OK")    != std::string::npos ||
                      lastResult.find("true")  != std::string::npos ||
                      lastResult.find("Moved") != std::string::npos;
            Color rc = ok ? t.colors.selectedBorder : t.colors.monsterStat;
            DrawText(lastResult.c_str(), (int)x, (int)ry, fsSub, rc);
        }
    }

private:
    static std::string zoneName(ZoneType type) {
        switch (type) {
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
        case ZoneType::None:         return "";
        }
        return "Unknown";
    }
};

} // namespace openjoey::ui
