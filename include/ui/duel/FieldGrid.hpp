#pragma once
#include "engine/field/Field.hpp"
#include "engine/field/zone/Zone.hpp"
#include <ui/cards/CardImageCache.hpp>
#include "ui/widgets/StyleSheet.hpp"
#include "ui/widgets/KeyboardNav2D.hpp"
#include "ui/duel/ZoneCell.hpp"
#include <algorithm>
#include <raylib.h>

namespace openjoey::ui {
using namespace openjoey::engine;
using cards::Card;
using cards::CardDatabase;
using zone::Field;
using zone::IZone;
using zone::Zone_Monster;
using zone::ZoneStack;
using zone::ZoneStack_Hand;

// Encapsulates the 6×9 zone grid for the duel field: cursor state, navigation,
// and all field-row + hand-row rendering. Action dispatch stays in DuelScreen.
class FieldGrid {
public:
    static constexpr int ROWS = 6, COLS = 9;

    // Populate grid_ and labels_ from a freshly constructed Field.
    // Zone labels, keyed by the owning player index (0 = first player "P1").
    static constexpr const char *kST[2][5] = {
        {"ST1-0", "ST1-1", "ST1-2", "ST1-3", "ST1-4"},
        {"ST2-0", "ST2-1", "ST2-2", "ST2-3", "ST2-4"},
    };
    static constexpr const char *kM[2][5] = {
        {"M1-0", "M1-1", "M1-2", "M1-3", "M1-4"},
        {"M2-0", "M2-1", "M2-2", "M2-3", "M2-4"},
    };
    static constexpr const char *kBan[2]  = {"BAN1", "BAN2"};
    static constexpr const char *kGy[2]   = {"GY1", "GY2"};
    static constexpr const char *kFld[2]  = {"FLD1", "FLD2"};
    static constexpr const char *kDk[2]   = {"DK1", "DK2"};
    static constexpr const char *kEd[2]   = {"ED1", "ED2"};
    static constexpr const char *kHand[2] = {"P1 Hand", "P2 Hand"};

    void build(Field& field) {
        // Row assignment follows the viewer: rows 1–2 are always the
        // opponent's side, rows 3–4 the viewer's own side (hotseat: the view
        // belongs to the turn player). Monster rows sit adjacent to the
        // mid-line (rows 2/3) like the physical mat: opponent monsters in
        // row 2, own monsters in row 3; spell/trap rows are the outer ones.
        const int me = viewer_, opp = 1 - viewer_;

        // Row 1 — opponent spell/trap row (outermost, mirrored left-right)
        grid_[1][1] = &field.deckZones[opp]; labels_[1][1] = kDk[opp];
        for (int i = 0; i < 5; ++i) {
            grid_[1][2 + i]   = &field.spellTrapZones[opp][4 - i];
            labels_[1][2 + i] = kST[opp][4 - i];
        }
        grid_[1][7] = &field.extraDeckZones[opp]; labels_[1][7] = kEd[opp];

        // Row 2 — opponent monster row (at the mid-line, mirrored)
        grid_[2][0] = &field.banishedZones[opp]; labels_[2][0] = kBan[opp];
        grid_[2][1] = &field.graveyardZones[opp]; labels_[2][1] = kGy[opp];
        for (int i = 0; i < 5; ++i) {
            grid_[2][2 + i]   = &field.monsterZones[opp][4 - i];
            labels_[2][2 + i] = kM[opp][4 - i];
        }
        grid_[2][7] = &field.fieldZones[opp]; labels_[2][7] = kFld[opp];

        // Row 3 — own monster row (at the mid-line)
        grid_[3][1] = &field.fieldZones[me]; labels_[3][1] = kFld[me];
        for (int i = 0; i < 5; ++i) {
            grid_[3][2 + i]   = &field.monsterZones[me][i];
            labels_[3][2 + i] = kM[me][i];
        }
        grid_[3][7] = &field.graveyardZones[me]; labels_[3][7] = kGy[me];
        grid_[3][8] = &field.banishedZones[me];  labels_[3][8] = kBan[me];

        // Row 4 — own spell/trap row (outermost)
        grid_[4][1] = &field.extraDeckZones[me]; labels_[4][1] = kEd[me];
        for (int i = 0; i < 5; ++i) {
            grid_[4][2 + i]   = &field.spellTrapZones[me][i];
            labels_[4][2 + i] = kST[me][i];
        }
        grid_[4][7] = &field.deckZones[me]; labels_[4][7] = kDk[me];
        // Rows 0 and 5 are hand rows — handled separately.
    }

    // Hotseat: the view belongs to the turn player. Rebuilds the row layout.
    void setViewer(int player, Field& field) {
        viewer_ = player;
        cursorRow_ = 3; cursorCol_ = 4; handCursor_ = 0;
        selectedZone_ = nullptr;
        build(field);
    }
    int viewer() const { return viewer_; }

    // ── State accessors (hands are viewer-relative: row 5 = own hand) ────────
    IZone* cursorZone(Field& field) const {
        if (cursorRow_ == 0) return &field.handZones[1 - viewer_];
        if (cursorRow_ == 5) return &field.handZones[viewer_];
        return grid_[cursorRow_][cursorCol_];
    }

    const char* cursorLabel(Field& field) const {
        (void)field; // hand labels come from the viewer index; kept for API symmetry
        if (cursorRow_ == 0) return kHand[1 - viewer_];
        if (cursorRow_ == 5) return kHand[viewer_];
        IZone* z = grid_[cursorRow_][cursorCol_];
        return z ? labels_[cursorRow_][cursorCol_] : "---";
    }

    // Top card of whatever the cursor sits on (hand rows included).
    Card* cursorCard(Field& field) const {
        if (cursorRow_ == 0) { // opponent hand (viewer-relative)
            auto& h = field.handZones[1 - viewer_];
            return h.isEmpty() ? nullptr : h.peek(handCursor_);
        }
        if (cursorRow_ == 5) { // own hand (viewer-relative)
            auto& h = field.handZones[viewer_];
            return h.isEmpty() ? nullptr : h.peek(handCursor_);
        }
        return peekZone(grid_[cursorRow_][cursorCol_]);
    }

    // Player owning a per-player zone; -1 for shared (Extra Monster Zones).
    int ownerOf(IZone* z, Field& field) const {
        if (!z) return -1;
        for (int p = 0; p < 2; ++p) {
            for (auto& z2 : field.monsterZones[p])   if (z == &z2) return p;
            for (auto& z2 : field.spellTrapZones[p]) if (z == &z2) return p;
            if (z == &field.handZones[p])      return p;
            if (z == &field.deckZones[p])      return p;
            if (z == &field.extraDeckZones[p]) return p;
            if (z == &field.graveyardZones[p]) return p;
            if (z == &field.banishedZones[p])  return p;
            if (z == &field.fieldZones[p])     return p;
        }
        return -1;
    }

    IZone*  selectedZone() const { return selectedZone_; }
    void    setSelectedZone(IZone* z) { selectedZone_ = z; }
    int     cursorRow()   const { return cursorRow_; }
    int     handCursor()  const { return handCursor_; }

    // ── Legal-target highlighting (set each frame by the screen) ─────────────
    // (zone pointer, outline color); drawn over the cell border.
    void clearHighlights() { highlights_.clear(); }
    void addHighlight(const IZone* z, Color c) {
        if (z) highlights_.push_back({z, c});
    }

    // ── Mouse hit-testing: map a screen point onto the cursor ────────────────
    // True when the point landed on a drawn zone/hand card; the cursor (and
    // hand cursor) is moved there, so every keyboard pathway (ENTER/menu/
    // targeting) works identically for mouse users.
    bool pointToCursor(Vector2 m, Field& field) {
        // Hand rows first (they overlap the top/bottom edges).
        for (int hi = 0; hi < 2; ++hi) {
            for (std::size_t i = 0; i < handRects_[hi].size(); ++i) {
                if (!CheckCollisionPointRec(m, handRects_[hi][i])) continue;
                cursorRow_  = hi == 0 ? 0 : ROWS - 1;
                handCursor_ = (int)i;
                return true;
            }
        }
        for (int r = 1; r <= 4; ++r)
            for (int c = 0; c < COLS; ++c) {
                Rectangle rc = cellRects_[r][c];
                if (rc.width <= 0 || !CheckCollisionPointRec(m, rc)) continue;
                cursorRow_ = r;
                cursorCol_ = c;
                (void)field;
                return true;
            }
        return false;
    }

    // ── Navigation ───────────────────────────────────────────────────────────
    // Hand rows are duel-specific (they scroll the hand, not the grid) and
    // stay here; field rows 1–4 delegate to the uikit KeyboardNav2D, which
    // owns the shared cursor math (skip-empty scan, nearest-occupied snap,
    // refuse-empty-row). One axis per call, exactly like the nav's contract.
    void moveCursor(int dr, int dc, Field& field) {
        if (cursorRow_ == 0 || cursorRow_ == 5) { // hand rows
            if (dc) {
                int p   = (cursorRow_ == 5) ? viewer_ : 1 - viewer_;
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
                        snapCol(); // entering the field: land on a zone
                }
            }
            return;
        }
        if (dr != 0) { // stepping off the field into a hand row
            const int nr = cursorRow_ + dr;
            if (nr == 0 || nr == ROWS - 1) {
                cursorRow_  = nr;
                handCursor_ = 0;
                return;
            }
        }
        // Field rows 1–4 in grid space = rows 0–3 in nav space.
        nav_.row = cursorRow_ - 1;
        nav_.col = cursorCol_;
        nav_.move(dr, dc, ROWS - 2, COLS, [this](int r, int c) {
            return grid_[r + 1][c] != nullptr;
        });
        cursorRow_ = nav_.row + 1;
        cursorCol_ = nav_.col;
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
                cellRects_[gridRow][col] = grid_[gridRow][col] ? r : Rectangle{};
                bool isCur = (cursorRow_ == gridRow && cursorCol_ == col);
                bool isSel = (grid_[gridRow][col] != nullptr &&
                              grid_[gridRow][col] == selectedZone_);
                if (grid_[gridRow][col])
                    ZoneCell::Draw(r, grid_[gridRow][col], labels_[gridRow][col],
                                   isCur, isSel, cache, cardBack);
                for (auto& [hz, hc] : highlights_)
                    if (grid_[gridRow][col] == hz)
                        DrawRectangleLinesEx({r.x - 2, r.y - 2, r.width + 4,
                                              r.height + 4}, 3.f, hc);
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

    static Card* peekZone(IZone* z) {
        if (!z) return nullptr;
        if (auto* zm = dynamic_cast<zone::Zone_Monster*>(z)) return zm->peek();
        if (auto* zs = dynamic_cast<zone::ZoneStack*>(z))    return zs->peek(-1);
        return nullptr;
    }
    const char* labels_[ROWS][COLS] = {};
    mutable Rectangle cellRects_[ROWS][COLS] = {};      // recorded by draw()
    mutable std::vector<Rectangle> handRects_[2] = {};  // [0]=opp row, [1]=own row
    mutable std::vector<std::pair<const IZone*, Color>> highlights_;
    int  viewer_      = 1;   // hotseat: which player the view belongs to
    int  cursorRow_   = 3;
    int  cursorCol_   = 4;
    IZone* selectedZone_ = nullptr;
    int  handCursor_  = 0;
    KeyboardNav2D nav_;      // shared uikit cursor math for field rows 1–4

    void snapCol() {
        const int best = KeyboardNav2D::nearestOccupiedCol(
            cursorRow_, cursorCol_, COLS,
            [this](int r, int c) { return grid_[r][c] != nullptr; });
        if (best >= 0) cursorCol_ = best;
    }

    static Rectangle cellRect(int col, int ry, int startX, int zoneW, int zoneH, int gapX) {
        int cx = startX + gapX + col * (zoneW + gapX);
        return {(float)cx, (float)ry, (float)zoneW, (float)zoneH};
    }

    void drawOppHand(Rectangle bounds, Field& field,
                     CardImageCache& cache, const Texture2D* cardBack) const {
        (void)cache; // opponent hand shows card backs only
        int fx = (int)bounds.x, fy = (int)bounds.y;
        int fw = (int)bounds.width, fh = (int)bounds.height;
        DrawRectangle(fx, fy, fw, fh, COLOR_BG_DARK);
        DrawLine(fx, fy + fh - 1, fx + fw, fy + fh - 1, COLOR_DIVIDER_LINE);

        ZoneStack_Hand& hand = field.handZones[1 - viewer_]; // never rendered open
        int cnt = hand.count();
        int fs  = FONT_CARD_STAT;
        handRects_[0].clear();
        DrawText(TextFormat("%s: %d (card backs)", kHand[1 - viewer_], cnt),
                 fx + MAIN_PAD_X, fy + (fh - fs) / 2, fs, COLOR_STAT_TEXT);
        if (cnt == 0) return;

        int cw = (int)(fh * 0.75f * (59.f / 86.f));
        int ch = (int)(fh * 0.75f);
        int totalW = cnt * (cw + 3) - 3;
        int startX = fx + (fw - totalW) / 2;
        for (int i = 0; i < cnt; ++i) {
            int cx  = startX + i * (cw + 3);
            int cy2 = fy + (fh - ch) / 2;
            handRects_[0].push_back({(float)cx, (float)cy2, (float)cw, (float)ch});
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
        (void)cardBack; // own hand renders faces (or fallbacks), never backs
        int fx = (int)bounds.x, fy = (int)bounds.y;
        int fw = (int)bounds.width, fh = (int)bounds.height;
        DrawRectangle(fx, fy, fw, fh, COLOR_BG_DARK);
        DrawLine(fx, fy, fx + fw, fy, COLOR_DIVIDER_LINE);

        ZoneStack_Hand& hand = field.handZones[viewer_]; // the viewer's hand
        int cnt = hand.count();
        int fs  = FONT_CARD_STAT;
        handRects_[1].clear();
        DrawText(TextFormat("%s: %d", kHand[viewer_], cnt),
                 fx + MAIN_PAD_X, fy + 3, fs, COLOR_STAT_TEXT);
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
            bool sel = (selectedZone_ == &field.handZones[viewer_]);
            Rectangle cr = {(float)cx, (float)cy2, (float)cw, (float)ch};
            handRects_[1].push_back(cr);

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
