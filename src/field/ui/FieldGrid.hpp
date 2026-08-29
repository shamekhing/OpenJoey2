#pragma once
#include "field/Field.hpp"
#include "zone/Zone.hpp"
#include "card/ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include "zone/ui/ZoneCell.hpp"
#include <algorithm>
#include <raylib.h>

namespace openjoey::ui {
using namespace openjoey::zone;

// Encapsulates the 6×9 zone grid for the duel field: cursor state, navigation,
// and all field-row + hand-row rendering. Action dispatch stays in DuelScreen.
class FieldGrid {
public:
    static constexpr int ROWS = 6, COLS = 9;

    // Populate grid_ and labels_ from a freshly constructed Field.
    void build(Field& field) {
        static constexpr const char *kST[2][5] = {
            {"ST2-0", "ST2-1", "ST2-2", "ST2-3", "ST2-4"},
            {"ST1-0", "ST1-1", "ST1-2", "ST1-3", "ST1-4"},
        };
        static constexpr const char *kM[2][5] = {
            {"M2-0", "M2-1", "M2-2", "M2-3", "M2-4"},
            {"M1-0", "M1-1", "M1-2", "M1-3", "M1-4"},
        };

        // Row 1 — P2 monster row (viewed from P1, so mirrored)
        grid_[1][0] = &field.banishedZones[0]; labels_[1][0] = "BAN2";
        grid_[1][1] = &field.graveyardZones[0]; labels_[1][1] = "GY2";
        for (int i = 0; i < 5; ++i) {
            grid_[1][2 + i]   = &field.monsterZones[0][4 - i];
            labels_[1][2 + i] = kM[0][4 - i];
        }
        grid_[1][7] = &field.fieldZones[0]; labels_[1][7] = "FLD2";

        // Row 2 — P2 spell/trap row
        grid_[2][1] = &field.deckZones[0]; labels_[2][1] = "DK2";
        for (int i = 0; i < 5; ++i) {
            grid_[2][2 + i]   = &field.spellTrapZones[0][4 - i];
            labels_[2][2 + i] = kST[0][4 - i];
        }
        grid_[2][7] = &field.extraDeckZones[0]; labels_[2][7] = "ED2";

        // Row 3 — P1 spell/trap row
        grid_[3][1] = &field.extraDeckZones[1]; labels_[3][1] = "ED1";
        for (int i = 0; i < 5; ++i) {
            grid_[3][2 + i]   = &field.spellTrapZones[1][i];
            labels_[3][2 + i] = kST[1][i];
        }
        grid_[3][7] = &field.deckZones[1]; labels_[3][7] = "DK1";

        // Row 4 — P1 monster row
        grid_[4][1] = &field.fieldZones[1]; labels_[4][1] = "FLD1";
        for (int i = 0; i < 5; ++i) {
            grid_[4][2 + i]   = &field.monsterZones[1][i];
            labels_[4][2 + i] = kM[1][i];
        }
        grid_[4][7] = &field.graveyardZones[1]; labels_[4][7] = "GY1";
        grid_[4][8] = &field.banishedZones[1];  labels_[4][8] = "BAN1";
        // Rows 0 and 5 are hand rows — handled separately.
    }

    // ── State accessors
    IZone* cursorZone(Field& field) const {
        if (cursorRow_ == 0) return &field.handZones[0];
        if (cursorRow_ == 5) return &field.handZones[1];
        return grid_[cursorRow_][cursorCol_];
    }

    const char* cursorLabel(Field& field) const {
        if (cursorRow_ == 0) return "P2 Hand";
        if (cursorRow_ == 5) return "P1 Hand";
        IZone* z = grid_[cursorRow_][cursorCol_];
        return z ? labels_[cursorRow_][cursorCol_] : "---";
    }

    IZone*  selectedZone() const { return selectedZone_; }
    void    setSelectedZone(IZone* z) { selectedZone_ = z; }
    int     cursorRow()   const { return cursorRow_; }
    int     handCursor()  const { return handCursor_; }

    // ── Navigation
    void moveCursor(int dr, int dc, Field& field) {
        bool inHand = (cursorRow_ == 0 || cursorRow_ == 5);
        if (inHand) {
            if (dc) {
                int p   = (cursorRow_ == 0) ? 0 : 1;
                int cnt = field.handZones[p].count();
                if (cnt > 0)
                    handCursor_ = (handCursor_ + dc + cnt) % cnt;
                return;
            }
            if (dr) {
                int nr = std::clamp(cursorRow_ + dr, 0, ROWS - 1);
                if (nr != cursorRow_) {
                    cursorRow_ = nr;
                    if (nr > 0 && nr < 5)
                        snapCol();
                }
            }
            return;
        }
        if (dr) {
            int nr = cursorRow_ + dr;
            if (nr < 0 || nr >= ROWS) return;
            if (nr == 0 || nr == 5) {
                cursorRow_   = nr;
                handCursor_  = 0;
                return;
            }
            int nc = cursorCol_;
            if (!grid_[nr][nc]) {
                for (int d = 1; d < COLS; ++d) {
                    if (nc - d >= 0 && grid_[nr][nc - d]) { nc = nc - d; break; }
                    if (nc + d < COLS && grid_[nr][nc + d]) { nc = nc + d; break; }
                }
            }
            if (grid_[nr][nc]) {
                cursorRow_ = nr;
                cursorCol_ = nc;
            }
            return;
        }
        if (dc) {
            int nc = cursorCol_ + dc;
            while (nc >= 0 && nc < COLS) {
                if (grid_[cursorRow_][nc]) { cursorCol_ = nc; return; }
                nc += dc;
            }
        }
    }

    // ── Draw the entire field area (center panel).
    void draw(Rectangle bounds, Field& field,
              CardImageCache& cache, const Texture2D* cardBack) const {
        int fx = (int)bounds.x, fy = (int)bounds.y;
        int fw = (int)bounds.width, fh = (int)bounds.height;

        DrawRectangle(fx, fy, fw, fh, COLOR_FIELD_MAT);

        int gapX  = fw * 5 / 1000;
        int gapY  = fh * 6 / 1000;
        int handH = fh * 11 / 100;
        int zoneH = (fh - handH * 2 - gapY * 5) / 4;
        int zoneW = (fw - gapX * (COLS + 1)) / COLS;
        if (zoneW > zoneH * 85 / 100) zoneW = zoneH * 85 / 100;

        int gridW  = COLS * zoneW + (COLS + 1) * gapX;
        int startX = fx + (fw - gridW) / 2;

        int rowY[4];
        rowY[0] = fy + handH + gapY;
        rowY[1] = rowY[0] + zoneH + gapY;
        rowY[2] = rowY[1] + zoneH + gapY;
        rowY[3] = rowY[2] + zoneH + gapY;

        drawOppHand({(float)fx, (float)fy, (float)fw, (float)handH}, field, cache, cardBack);

        for (int gridRow = 1; gridRow <= 4; ++gridRow) {
            DrawRectangle(startX, rowY[gridRow - 1], gridW, zoneH,
                          Fade(COLOR_BG_MAIN, 0.85f));
            for (int col = 0; col < COLS; ++col) {
                Rectangle r = cellRect(col, rowY[gridRow - 1], startX, zoneW, zoneH, gapX);
                bool isCur = (cursorRow_ == gridRow && cursorCol_ == col);
                bool isSel = (grid_[gridRow][col] != nullptr &&
                              grid_[gridRow][col] == selectedZone_);
                if (grid_[gridRow][col])
                    ZoneCell::Draw(r, grid_[gridRow][col], labels_[gridRow][col],
                                   isCur, isSel, cache, cardBack);
            }
        }

        // Divider between P2 and P1 spell/trap rows
        int divY = rowY[1] + zoneH + gapY / 2;
        DrawLineEx({(float)startX, (float)divY},
                   {(float)(startX + gridW), (float)divY}, 2.f, COLOR_DIVIDER_MID);

        drawOwnHand({(float)fx, (float)(fy + fh - handH), (float)fw, (float)handH},
                    field, cache, cardBack);
    }

private:
    IZone*      grid_[ROWS][COLS]   = {};
    const char* labels_[ROWS][COLS] = {};
    int  cursorRow_   = 4;
    int  cursorCol_   = 4;
    IZone* selectedZone_ = nullptr;
    int  handCursor_  = 0;

    void snapCol() {
        if (grid_[cursorRow_][cursorCol_]) return;
        for (int d = 1; d < COLS; ++d) {
            if (cursorCol_ - d >= 0 && grid_[cursorRow_][cursorCol_ - d]) {
                cursorCol_ -= d; return;
            }
            if (cursorCol_ + d < COLS && grid_[cursorRow_][cursorCol_ + d]) {
                cursorCol_ += d; return;
            }
        }
    }

    static Rectangle cellRect(int col, int ry, int startX, int zoneW, int zoneH, int gapX) {
        int cx = startX + gapX + col * (zoneW + gapX);
        return {(float)cx, (float)ry, (float)zoneW, (float)zoneH};
    }

    void drawOppHand(Rectangle bounds, Field& field,
                     CardImageCache& cache, const Texture2D* cardBack) const {
        int fx = (int)bounds.x, fy = (int)bounds.y;
        int fw = (int)bounds.width, fh = (int)bounds.height;
        DrawRectangle(fx, fy, fw, fh, COLOR_BG_DARK);
        DrawLine(fx, fy + fh - 1, fx + fw, fy + fh - 1, COLOR_DIVIDER_LINE);

        int cnt = field.handZones[0].count();
        int fs  = FONT_CARD_STAT;
        DrawText(TextFormat("P2 Hand: %d", cnt), fx + MAIN_PAD_X,
                 fy + (fh - fs) / 2, fs, COLOR_STAT_TEXT);
        if (cnt == 0) return;

        int cw = (int)(fh * 0.75f * (59.f / 86.f));
        int ch = (int)(fh * 0.75f);
        int totalW = cnt * (cw + 3) - 3;
        int startX = fx + (fw - totalW) / 2;
        for (int i = 0; i < cnt; ++i) {
            int cx  = startX + i * (cw + 3);
            int cy2 = fy + (fh - ch) / 2;
            if (cardBack && cardBack->id)
                DrawTexturePro(*cardBack, {0, 0, (float)cardBack->width, (float)cardBack->height},
                               {(float)cx, (float)cy2, (float)cw, (float)ch}, {0, 0}, 0.f, WHITE);
            else {
                DrawRectangle(cx, cy2, cw, ch, COLOR_BG_DARK);
                DrawRectangleLines(cx, cy2, cw, ch, Color{210, 170, 40, 255});
            }
        }
    }

    void drawOwnHand(Rectangle bounds, Field& field,
                     CardImageCache& cache, const Texture2D* cardBack) const {
        int fx = (int)bounds.x, fy = (int)bounds.y;
        int fw = (int)bounds.width, fh = (int)bounds.height;
        DrawRectangle(fx, fy, fw, fh, COLOR_BG_DARK);
        DrawLine(fx, fy, fx + fw, fy, COLOR_DIVIDER_LINE);

        // field.handZones[1] is non-const but field is non-const here
        ZoneStack_Hand& hand = field.handZones[1];
        int cnt = hand.count();
        int fs  = FONT_CARD_STAT;
        DrawText(TextFormat("P1 Hand: %d", cnt), fx + MAIN_PAD_X, fy + 3, fs, COLOR_STAT_TEXT);
        if (cnt == 0) {
            DrawText("(empty)", fx + fw / 2, fy + (fh - fs) / 2, fs, DARKGRAY);
            return;
        }

        int cw = (int)(fh * 0.85f * (59.f / 86.f));
        int ch = (int)(fh * 0.85f);
        int totalW = cnt * (cw + 4) - 4;
        int startX = fx + (fw - totalW) / 2;
        for (int i = 0; i < cnt; ++i) {
            Card* c = hand.peek(i);
            if (!c) continue;
            int cx  = startX + i * (cw + 4);
            int cy2 = fy + (fh - ch) - 4;
            bool cur = (cursorRow_ == 5 && handCursor_ == i);
            bool sel = (selectedZone_ == &field.handZones[1]);
            Rectangle cr = {(float)cx, (float)cy2, (float)cw, (float)ch};

            const Texture2D* tex = cache.Get(*c);
            if (tex && tex->id) {
                DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                               cr, {0, 0}, 0.f, WHITE);
            } else {
                Color fc = c->isMonster() ? COLOR_MONSTER_STAT
                           : c->isSpell() ? COLOR_SPELL_STAT
                                          : COLOR_TRAP_STAT;
                DrawRectangleRec(cr, Fade(fc, 0.6f));
                DrawText(c->name.substr(0, 6).c_str(), (int)cx + 2, (int)cy2 + 2,
                         FONT_HELP_TEXT, WHITE);
            }
            float thick  = (cur || sel) ? 2.5f : 1.f;
            Color border = cur ? YELLOW : sel ? GREEN : Color{180, 180, 210, 200};
            DrawRectangleLinesEx(cr, thick, border);
        }
    }
};

} // namespace openjoey::ui
