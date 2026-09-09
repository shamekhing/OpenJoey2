#pragma once
// ── KeyboardNav2D (openjoey::ui) ─────────────────────────────────────────────
// 2-D counterpart to KeyboardNav (widgets/input/KeyboardNav.hpp): a reusable
// (row, col) cursor for keyboard-driven boards. Pure cursor math — no raylib,
// no drawing, no grid storage — so it is headless-testable like the rest of
// the uikit input widgets. FieldGrid supplies the occupancy predicate and
// keeps only its truly duel-specific behaviors (hand-row scrolling).

#include <functional>

namespace openjoey::ui {

// Movement feel, matching the original duel-field cursor:
//   * a horizontal step lands on the immediate neighbour; if it is empty the
//     cursor scans ahead in the travel direction to the row's edge; a row
//     with no occupied cell ahead refuses the move;
//   * a vertical step refuses when the whole target row is empty, otherwise
//     the column snaps to the nearest occupied cell (ties → lower column).
class KeyboardNav2D {
   public:
    // occupancy(row, col) — must be valid for every cell in bounds.
    using Occupied = std::function<bool(int row, int col)>;

    int row = 0;
    int col = 0;

    void reset(int r = 0, int c = 0) {
        row = r;
        col = c;
    }

    // Move by (dr, dc) — one axis per call, like the duel cursor. Bounds are
    // [0, rows) × [0, cols); degenerate dimensions disable movement.
    void move(int dr, int dc, int rows, int cols, const Occupied& occupied) {
        if (rows <= 0 || cols <= 0 || !occupied) return;
        if (dc != 0) moveHoriz(dc, cols, occupied);
        else if (dr != 0) moveVert(dr, rows, cols, occupied);
    }

    // Nearest occupied column in row r seen from `from` (ties → lower
    // column); -1 when the row has no occupied cell. Shared by moveVert and
    // by callers that snap in place (e.g. the duel FieldGrid entering the
    // field from a hand row).
    static int nearestOccupiedCol(int r, int from, int cols, const Occupied& occ) {
        int best = -1, bd = cols + 1;
        for (int c = 0; c < cols; ++c)
            if (occ(r, c)) {
                int d = c > from ? c - from : from - c;
                if (d < bd) {
                    bd = d;
                    best = c;
                }
            }
        return best;
    }

   private:
    void moveHoriz(int dc, int cols, const Occupied& occ) {
        // Scan the full row in the travel direction; refuse when nothing is
        // occupied ahead (never jumps backwards).
        int nc = col + dc;
        while (nc >= 0 && nc < cols) {
            if (occ(row, nc)) {
                col = nc;
                return;
            }
            nc += dc;
        }
    }

    void moveVert(int dr, int rows, int cols, const Occupied& occ) {
        int nr = row + dr;
        if (nr < 0 || nr >= rows) return;
        int best = nearestOccupiedCol(nr, col, cols, occ);
        if (best < 0) return;  // empty row — cursor stays put
        row = nr;
        col = best;
    }
};

}  // namespace openjoey::ui