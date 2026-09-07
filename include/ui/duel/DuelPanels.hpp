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

    // ── Overlays: handoff gate, chain window, win banner, help panel ─────────

    // ── Overlays: handoff gate, chain window, win banner, help panel ─────────
    static void drawOverlays(const Duel& duel, const DuelUIState& st) {
        if (st.handoff) {
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 220});
            const char* msg =
                TextFormat("PASS THE DEVICE TO PLAYER %d", duel.turnPlayer + 1);
            DrawText(msg, _SW / 2 - MeasureText(msg, TITLE_FONT_SIZE) / 2,
                     _SH / 2 - TITLE_FONT_SIZE, TITLE_FONT_SIZE, RAYWHITE);
            const char* sub = "press SPACE when you are ready (hides both hands)";
            DrawText(sub, _SW / 2 - MeasureText(sub, FONT_HELP_SMALL) / 2, _SH / 2,
                     FONT_HELP_SMALL, GRAY);
            return;
        }
        if (duel.result != DuelResult::Ongoing) {
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 200});
            const bool p1 = duel.result == DuelResult::Player0Win;
            const char* msg = p1 ? "PLAYER 1 WINS!" : "PLAYER 2 WINS!";
            const int fs = FONT_MAIN_TITLE;
            DrawText(msg, _SW / 2 - MeasureText(msg, fs) / 2, _SH / 2 - fs,
                     fs, p1 ? GOLD : SKYBLUE);
            const char* sub = "press R for a rematch";
            DrawText(sub, _SW / 2 - MeasureText(sub, FONT_CARD_NAME) / 2,
                     _SH / 2 + FONT_CARD_NAME, FONT_CARD_NAME, RAYWHITE);
            return;
        }
        if (st.chainPrompt) {
            const char* msg = TextFormat(
                "Chain open (%d link%s) — the other player may respond "
                "(activate a set card) · R = resolve",
                (int)duel.chain.links.size(),
                duel.chain.links.size() == 1 ? "" : "s");
            const int fs = FONT_CARD_NAME;
            const int bw = MeasureText(msg, fs) + 40;
            DrawRectangle(_SW / 2 - bw / 2, 8, bw, fs + 12, {20, 20, 30, 230});
            DrawText(msg, _SW / 2 - MeasureText(msg, fs) / 2, 14, fs, ORANGE);
        }
        if (st.helpOpen) {  // modal controls panel — H closes it
            DrawRectangle(0, 0, _SW, _SH, {0, 0, 0, 200});
            const int pw = _SW * 52 / 100, ph = _SH * 60 / 100;
            const int px = _SW / 2 - pw / 2, py = _SH / 2 - ph / 2;
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
        }
    }
};

}  // namespace openjoey::ui