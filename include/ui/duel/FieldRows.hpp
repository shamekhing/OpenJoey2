#pragma once
// ── Semantic row indices for the duel field grid (openjoey::ui) ─────────────
// Row order is viewer-relative (hotseat: rows 1–2 are always the opponent's
// side, rows 3–4 the viewer's own; monster rows sit adjacent at the mid-line
// like the physical mat). DuelScreen and FieldGrid address rows through these
// names — raw row literals belong only in FieldRows and FieldGrid.
//
//   0  opponent hand strip (face-down backs)
//   1  opponent spell/trap row (outer)   + opp deck / extra deck flanks
//   2  opponent monster row (mid-line)   + banish / GY / field flanks
//   3  own monster row (mid-line)        + field / GY / banish flanks
//   4  own spell/trap row (outer)        + extra deck / deck flanks
//   5  own hand strip

namespace openjoey::ui {
using namespace openjoey::engine;

enum class FieldRow : int {
    OppHand = 0,
    OppST = 1,
    OppMonster = 2,
    OwnMonster = 3,
    OwnST = 4,
    OwnHand = 5,
};

constexpr int kFieldRows = 6;  // hand strips included
constexpr int kFieldCols = 9;

constexpr int fieldRow(FieldRow r) { return static_cast<int>(r); }

constexpr bool isHandRow(int r) { return r == fieldRow(FieldRow::OppHand) || r == fieldRow(FieldRow::OwnHand); }
constexpr bool isOppMonsterRow(int r) { return r == fieldRow(FieldRow::OppMonster); }
constexpr bool isOwnMonsterRow(int r) { return r == fieldRow(FieldRow::OwnMonster); }
constexpr bool isMonsterRow(int r) { return isOppMonsterRow(r) || isOwnMonsterRow(r); }

}  // namespace openjoey::ui