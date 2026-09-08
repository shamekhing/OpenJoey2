#pragma once
// ── Duel screen chrome + overlays (openjoey::ui) ─────────────────────────────
// Header/footer strips, phase name, and the modal overlays (pass-device
// gate, win banner, chain banner, help panel). Pure drawing: all sizes are
// derived from the ui/widgets StyleSheet — no fixed pixel values, no
// state (the header hint reads the shared DuelUIState).

#include <raylib.h>

#include <string>

#include "engine/duel/Duel.hpp"
#include "ui/duel/Action.hpp"
#include "ui/duel/DuelLayout.hpp"
#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {
using namespace openjoey::engine;

struct DuelPanels {
    static const char* phaseName(Phase p) {
        switch (p) {
            case Phase::Draw:
                return "Draw";
            case Phase::Standby:
                return "Standby";
            case Phase::Main1:
                return "Main 1";
            case Phase::Battle:
                return "Battle";
            case Phase::Main2:
                return "Main 2";
            case Phase::End:
                return "End";
        }
        return "?";
    }

    // Top strip: LP / turn / phase (left) and a state hint (center/right).
    static void drawHeader(const Engine& engine, const Duel& duel,
                           const DuelUIState& st, int x, int y, int w, int h) {
        DrawRectangle(x, y, w, h, COLOR_HEADER_BG);
        DrawLine(x, y + h - 1, x + w, y + h - 1, COLOR_DIVIDER_LINE);
        int fs = FONT_SCREEN_TITLE;

        std::string l = "P1 " + std::to_string(engine.lp(0)) + "   P2 " +
                        std::to_string(engine.lp(1)) + "   T:P" +
                        std::to_string(duel.turnPlayer + 1) + "  " +
                        phaseName(duel.turn.phase);
        DrawText(l.c_str(), x + HEADER_TITLE_X, y + (h - fs) / 2, fs, COLOR_STAT_TEXT);

        // Phase timeline (right side): Draw · MP1 · BP · MP2 · End — current lit.
        {
            static constexpr const char* kPhases[] = {"Draw", "MP1", "BP", "MP2", "End"};
            const Phase kMap[] = {Phase::Draw, Phase::Main1, Phase::Battle,
                                  Phase::Main2, Phase::End};
            const int pw = 46, gap = 8, fs2 = 16, py = y + (h - fs2) / 2;
            int px = x + w - (5 * pw + 4 * gap) - 20;
            for (int i = 0; i < 5; ++i) {
                bool cur = (duel.turn.phase == kMap[i]);
                Color bg = cur ? Color{255, 220, 0, 70} : Color{70, 70, 95, 120};
                Color fg = cur ? YELLOW : Color{150, 150, 170, 220};
                Rectangle pr = {(float)px, (float)py - 4, (float)pw, (float)fs2 + 8};
                DrawRectangleRec(pr, bg);
                DrawRectangleLinesEx(pr, 1.f, cur ? YELLOW : Color{90, 90, 115, 160});
                int tw = MeasureText(kPhases[i], fs2);
                DrawText(kPhases[i], px + (pw - tw) / 2, py, fs2, fg);
                px += pw + gap;
            }
        }

        const char* msg = nullptr;
        Color mc = YELLOW;
        if (duel.result != DuelResult::Ongoing) {
            msg = duel.result == DuelResult::Draw
                      ? "DRAW!  [R = rematch]"
                      : (duel.result == DuelResult::Player0Win
                             ? "PLAYER 1 WINS!  [R = rematch]"
                             : "PLAYER 2 WINS!  [R = rematch]");
            mc = GOLD;
        } else if (st.handoff) {
            msg = "PASS THE DEVICE  [SPACE = start turn]";
        } else if (st.chainPrompt) {
            msg = "CHAIN WINDOW  [R = resolve, or chain a set card]";
        } else if (st.mode == DuelMode::Menu) {
            msg = "ACTION?  [ENTER = run, ESC = back]";
        } else if (st.mode == DuelMode::AttackTarget) {
            msg = "PICK TARGET  [ENTER = confirm, ESC = cancel]";
        } else if (st.mode == DuelMode::EffectTarget ||
                   st.mode == DuelMode::TributeTarget) {
            msg = "PICK TARGET  [ENTER = pick, F = confirm tributes, ESC = cancel]";
        } else {
            msg = "NAVIGATE  [ENTER = menu, SPACE = attack, H = help]";
        }
        DrawText(msg, x + w / 2 - MeasureText(msg, fs) / 2, y + (h - fs) / 2, fs, mc);
    }

    // Bottom strip: one-line control cheatsheet.
    static void drawFooter(int x, int y, int w, int h) {
        DrawRectangle(x, y, w, h, COLOR_FOOTER_BG);
        DrawLine(x, y, x + w, y, COLOR_DIVIDER_LINE);
        int fs = FONT_HELP_TEXT;
        DrawText(
            "Mouse:click=use  R-click=cancel  |  Arrows/WASD:move  Enter:use  "
            "SPACE:attack  B:Battle  N:Main2  C:position  F:flip/tributes  "
            "E:end  R:resolve/rematch  Z:undo  L:log  H:help",
            x + MAIN_PAD_X, y + (h - fs) / 2, fs, COLOR_STAT_TEXT);
    }

    // ── Action bar: the touch replacement for the B / N / E / Z / L keys ─────
    // Slots: 0 = phase action (BATTLE or MAIN 2) · 1 = END TURN · 2 = UNDO ·
    // 3 = LOG. Labels and enabled states live here so Draw() and
    // DuelScreen::handleInput() agree through the same functions.
    static const char* barLabel(const Duel& duel, const DuelUIState& st, int slot) {
        switch (slot) {
            case 0:
                return duel.turn.phase == Phase::Battle ? "MAIN 2" : "BATTLE";
            case 1:
                return "END TURN";
            case 2:
                return "UNDO";
            case 3:
                return "LOG";
            default:
                return st.hideHand ? "PEEK" : "HIDE";
        }
    }
    static bool barEnabled(const Engine& engine, const Duel& duel, int slot) {
        if (duel.result != DuelResult::Ongoing) return slot == 3;
        switch (slot) {
            case 0:
                if (duel.turn.phase == Phase::Main1) return !duel.turn.skipBattle;
                if (duel.turn.phase == Phase::Battle) return true;
                return false;
            case 1:
                return duel.canAct();
            case 2:
                return engine.canUndo();
            default:
                return true;
        }
    }
    static void drawOverlayButton(const Rectangle& r, const char* label,
                                  Color border, Color text) {
        DrawRectangleRec(r, COLOR_PANEL_BG);
        DrawRectangleLinesEx(r, 2.f, border);
        const int fs = FONT_PANEL_TITLE;
        DrawText(label, (int)(r.x + (r.width - MeasureText(label, fs)) / 2),
                 (int)(r.y + (r.height - fs) / 2), fs, text);
    }
    static void drawActionBar(const Engine& engine, const Duel& duel,
                              const DuelUIState& st) {
        const Rectangle bar = DuelLayout::barRect();
        DrawRectangleRec(bar, COLOR_HEADER_BG);
        DrawLine((int)bar.x, (int)bar.y, (int)(bar.x + bar.width), (int)bar.y,
                 COLOR_DIVIDER_LINE);
        const int fs = 0.028f * _SH < 14 ? 14 : (int)(0.028f * _SH);
        for (int slot = 0; slot < DuelLayout::kBarButtons; ++slot) {
            const Rectangle r = DuelLayout::barButton(slot);
            const bool on = barEnabled(engine, duel, slot);
            const bool active = slot == 3 && st.logOpen;
            Color fg = !on      ? Color{105, 105, 128, 255}
                       : active ? GOLD
                                : RAYWHITE;
            Rectangle inner{r.x + 4, r.y + 5, r.width - 8, r.height - 10};
            DrawRectangleRec(inner, on ? COLOR_PANEL_BG : Fade(COLOR_PANEL_BG, 0.55f));
            DrawRectangleLinesEx(inner, 1.5f,
                                 active ? GOLD
                                 : on   ? COLOR_PANEL_BORDER
                                        : Fade(COLOR_PANEL_BORDER, 0.4f));
            const char* label = barLabel(duel, st, slot);
            DrawText(label, (int)(r.x + (r.width - MeasureText(label, fs)) / 2),
                     (int)(r.y + (r.height - fs) / 2), fs, fg);
        }
    }

    // ── Overlays: handoff gate, chain window, win banner, help panel ─────────

    // ── Overlays: handoff gate, chain window, win banner, help panel ─────────
    // Chain banner geometry shared by the overlay draw and input hit-testing.
    static Rectangle chainBannerRect(const Duel& duel) {
        const char* msg =
            TextFormat(
                "Chain open (%d link%s) — the other player may respond "
                "(activate a set card)",
                (int)duel.chain.links.size(),
                duel.chain.links.size() == 1 ? "" : "s");
        const int fs = FONT_CARD_NAME;
        const float btnW = DuelLayout::bannerButton(0, 0, 0, 0).width + 12.f;
        const float bw = (float)MeasureText(msg, fs) + 40.f + btnW;
        return {(float)_SW / 2.f - bw / 2, 8.f, bw, (float)fs + 12.f};
    }
    static Rectangle chainButtonRect(const Duel& duel) {
        const Rectangle b = chainBannerRect(duel);
        return DuelLayout::bannerButton(b.x, b.width, b.y, b.height);
    }
    static void drawOverlays(const Duel& duel, const DuelUIState& st) {
        if (st.handoff) {
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 220});
            const char* msg =
                TextFormat("PASS THE DEVICE TO PLAYER %d", duel.turnPlayer + 1);
            DrawText(msg, _SW / 2 - MeasureText(msg, TITLE_FONT_SIZE) / 2,
                     _SH / 2 - TITLE_FONT_SIZE, TITLE_FONT_SIZE, RAYWHITE);
            const char* sub =
                "your hands stay hidden until you claim the turn (SPACE works too)";
            DrawText(sub, _SW / 2 - MeasureText(sub, FONT_HELP_SMALL) / 2, _SH / 2,
                     FONT_HELP_SMALL, GRAY);
            drawOverlayButton(DuelLayout::handoffButton(),
                              "I'M PLAYER 2 — SHOW MY TURN", GOLD, RAYWHITE);
            return;
        }
        if (duel.result != DuelResult::Ongoing) {
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 200});
            const bool p1 = duel.result == DuelResult::Player0Win;
            const char* msg = p1 ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            const int fs = FONT_MAIN_TITLE;
            DrawText(msg, _SW / 2 - MeasureText(msg, fs) / 2, _SH / 2 - fs,
                     fs, p1 ? GOLD : SKYBLUE);
            const char* sub = "or press R for a rematch";
            DrawText(sub, _SW / 2 - MeasureText(sub, FONT_CARD_NAME) / 2,
                     _SH / 2 + FONT_CARD_NAME, FONT_CARD_NAME, RAYWHITE);
            drawOverlayButton(DuelLayout::winButton(0), "REMATCH", GOLD, RAYWHITE);
            drawOverlayButton(DuelLayout::winButton(1), "MAIN MENU",
                              COLOR_PANEL_BORDER, RAYWHITE);
            return;
        }
        if (st.chainPrompt) {
            const char* msg = TextFormat(
                "Chain open (%d link%s) — the other player may respond "
                "(activate a set card)",
                (int)duel.chain.links.size(),
                duel.chain.links.size() == 1 ? "" : "s");
            const int fs = FONT_CARD_NAME;
            const bool waiting =
                duel.config.chainResponseWindow && duel.chain.consecutivePasses < 2;
            const Rectangle banner = chainBannerRect(duel);
            DrawRectangleRec(banner, {20, 20, 30, 230});
            DrawText(msg, (int)(banner.x + 20), 14, fs, ORANGE);
            drawOverlayButton(chainButtonRect(duel), waiting ? "PASS" : "RESOLVE",
                              ORANGE, RAYWHITE);
        }
        if (st.helpOpen) {  // modal controls panel — H or GOT IT closes it
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 200});
            const Rectangle panel = DuelLayout::helpPanelRect();
            const int px = (int)panel.x, py = (int)panel.y;
            const int pw = (int)panel.width, ph = (int)panel.height;
            DrawRectangle(px, py, pw, ph, {18, 20, 28, 250});
            DrawRectangleLinesEx({(float)px, (float)py, (float)pw, (float)ph}, 2, GOLD);
            int y = py + ph * 4 / 100;
            auto line = [&](const char* l, Color c) {
                DrawText(l, px + pw * 36 / 1000, y, FONT_PANEL_TITLE, c);
                y += FONT_PANEL_TITLE * 14 / 10;
            };
            line("OPENJOY DUEL - CONTROLS   (H to close)", GOLD);
            line("MOUSE  click a card/zone = open menu, pick targets,", SKYBLUE);
            line("       right-click = cancel, hover = inspect", SKYBLUE);
            line("Arrows / WASD    move the cursor", RAYWHITE);
            line("ENTER            open the action menu / confirm", RAYWHITE);
            line("ESC              cancel menu, targeting or attack", RAYWHITE);
            line("SPACE            attack with selected monster (Battle Phase)", RAYWHITE);
            line("B / N            go to Battle Phase / Main Phase 2", RAYWHITE);
            line("C                change battle position (once per turn)", RAYWHITE);
            line("F                flip summon / confirm tribute-fusion picks", RAYWHITE);
            line("E                end turn (hand limit is enforced)", RAYWHITE);
            line("R                resolve the chain / rematch after game over", RAYWHITE);
            line("L                toggle the duel log (full narration)", RAYWHITE);
            line("Z                undo the last action", RAYWHITE);
            line("SPACE at gate    confirm the pass-device handoff", RAYWHITE);
            line("", RAYWHITE);
            line("FLOW: Main1 summon/set/activate -> B -> Battle attack -> N -> E", GRAY);
            line("Direct attacks only vs an empty field. Tributes: Lv5-6 = 1, Lv7+ = 2.", GRAY);
            drawOverlayButton(DuelLayout::helpOkButton(), "GOT IT", GOLD, RAYWHITE);
        }
    }
};

}  // namespace openjoey::ui