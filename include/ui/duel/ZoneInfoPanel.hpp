#pragma once
#include <raylib.h>

#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "engine/field/zone/Zones.hpp"
#include "ui/duel/Action.hpp"
#include "ui/widgets/DrawUtils.hpp"
#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;

// Right-side panel: zone state readout + the verdict strip. The action list
// itself moved to the tappable bottom action sheet (DuelScreen), which is
// shared by keyboard, mouse and touch.
struct ZoneInfoPanel {
    static void Draw(Rectangle r, zone::IZone* zone, const char* label, DuelUIState::Feedback feedback, const std::string& lastResult, bool hasSource) {
        DrawRectangleRec(r, COLOR_PANEL_BG);
        DrawRectangleLinesEx(r, 1.f, COLOR_PANEL_BORDER);

        float pad = r.width * 0.06f;
        float x = r.x + pad;
        float maxW = r.width - pad * 2;
        float cy = r.y + pad;

        int fsTitle = std::max(12, (int)(r.height * 0.032f));
        int fsSub = std::max(9, (int)(r.height * 0.024f));
        int fsSmall = std::max(8, (int)(r.height * 0.020f));
        float lineH = fsSub * 1.5f;

        DrawText(label, (int)x, (int)cy, fsTitle, YELLOW);
        cy += fsTitle + pad * 0.5f;

        std::string info = zoneName(zone->type());
        info += "  [" + std::to_string(zone->count()) + "]";
        info += zone->isEmpty() ? "  empty" : "  occupied";
        DrawText(info.c_str(), (int)x, (int)cy, fsSub, COLOR_STAT_TEXT);
        cy += lineH;

        if (auto* zm = dynamic_cast<zone::Zone_Monster*>(zone)) {
            std::string ori = zm->position() == zone::Orientation::Vertical ? "ATK" : "DEF";
            std::string vis;
            switch (zm->visibility()) {
                case zone::Visibility::Visible: vis = "FaceUp (both)"; break;
                case zone::Visibility::Limited: vis = "FaceDown (you know it)"; break;
                case zone::Visibility::Restricted: vis = "Hidden (neither)"; break;
            }
            DrawText(("Pos: " + ori).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY);
            cy += lineH;
            DrawText(("Vis: " + vis).c_str(), (int)x, (int)cy, fsSub, LIGHTGRAY);
            cy += lineH;
        }
        cy += pad * 0.5f;

        if (hasSource) {
            DrawRectangleRec({r.x, cy - 2, r.width, fsSub + 6.f}, Fade(GREEN, 0.18f));
            DrawText("* SOURCE SELECTED *", (int)x, (int)cy, fsSub, GREEN);
            cy += lineH;
        }

        DrawLine((int)(r.x + pad * 0.5f), (int)cy, (int)(r.x + r.width - pad * 0.5f), (int)cy, COLOR_DIVIDER_LINE);
        cy += pad * 0.5f;

        // (The numbered action list lives in the bottom action sheet now —
        // one menu UI for keyboard, mouse and touch instead of two.)
        (void)x;
        (void)maxW;
        (void)fsSmall;

        if (!lastResult.empty()) {
            float ry = r.y + r.height - fsSub * 3.0f;
            DrawLine((int)(r.x + pad * 0.5f), (int)ry, (int)(r.x + r.width - pad * 0.5f), (int)ry, COLOR_DIVIDER_LINE);
            ry += pad * 0.3f;
            // Verdict colour comes from the structured engine result, not from
            // sniffing the text.
            using FB = DuelUIState::Feedback;
            Color col = feedback == FB::Ok ? GREEN : feedback == FB::Fail ? Color{220, 80, 80, 255} : COLOR_STAT_TEXT;
            DrawText(lastResult.c_str(), (int)x, (int)ry, fsSub, col);
        }
    }

   private:
    static std::string zoneName(zone::ZoneType t) {
        switch (t) {
            case zone::ZoneType::Monster: return "Monster";
            case zone::ZoneType::SpellTrap: return "Spell/Trap";
            case zone::ZoneType::Field: return "Field";
            case zone::ZoneType::ExtraMonster: return "Extra Monster";
            case zone::ZoneType::Hand: return "Hand";
            case zone::ZoneType::Deck: return "Deck";
            case zone::ZoneType::ExtraDeck: return "Extra Deck";
            case zone::ZoneType::Graveyard: return "Graveyard";
            case zone::ZoneType::Banished: return "Banished";
            case zone::ZoneType::SideDeck: return "Side Deck";
        }
        return "Unknown";
    }
};

}  // namespace openjoey::ui
