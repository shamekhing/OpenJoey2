#pragma once
// ── Shared geometry for every tappable control on the duel screen ────────────
// Pure rect math. Draw() renders INTO these rects and handleInput() hit-tests
// THE SAME rects, so a button's drawn area is always exactly its hit area.
// Everything scales with the screen and every interactive rect respects the
// 44pt minimum touch target (Apple HIG) / 48dp (Material).
#include <raylib.h>

#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {

struct DuelLayout {
    // ── Action bar (bottom strip) ────────────────────────────────────────────
    // Slots: 0 = phase action (BATTLE / MAIN 2) · 1 = END TURN · 2 = UNDO ·
    // 3 = LOG · 4 = HAND privacy. The bar overlays the bottom of the duel
    // layout; DuelScreen shrinks the field by barH() so nothing underneath is
    // hidden.
    static constexpr int kBarButtons = 5;
    static float barH() {
        const float h = 0.07f * _SH;
        return h < 48.f ? 48.f : h;
    }
    static Rectangle barRect() {
        return {0.f, _SH - barH(), (float)_SW, barH()};
    }
    static Rectangle barButton(int slot) {
        const float w = _SW / (float)kBarButtons;
        return {slot * w, barRect().y, w, barH()};
    }

    // ── Cancel (✕) — visible in every non-Navigate mode ──────────────────────
    // Touch replacement for right-click / ESC; the keyboard paths stay live.
    static float cancelSize() {
        const float s = 0.06f * _SH;
        return s < 48.f ? 48.f : s;
    }
    static Rectangle cancelRect() {
        return {8.f, (float)HEADER_HEIGHT + 8.f, cancelSize(), cancelSize()};
    }

    // ── Action sheet: the tap/click rendering of the zone action menu ────────
    static float sheetRowH() {
        const float h = 0.06f * _SH;
        return h < 48.f ? 48.f : h;
    }
    static int sheetMaxRows() {
        const float avail = barRect().y - (float)HEADER_HEIGHT - 12.f;
        int rows = (int)(avail / sheetRowH());
        return rows < 1 ? 1 : (rows > 6 ? 6 : rows);
    }
    static Rectangle sheetRect(int visibleRows) {
        const float h = (float)visibleRows * sheetRowH();
        return {0.f, barRect().y - h, (float)_SW, h};
    }
    static Rectangle sheetRowRect(int visibleRows, int row) {
        const Rectangle s = sheetRect(visibleRows);
        return {0.f, s.y + (float)row * sheetRowH(), (float)_SW, sheetRowH()};
    }

    // ── Overlays ─────────────────────────────────────────────────────────────
    // Help modal panel (DuelPanels draws it; input hit-tests the GOT IT row).
    static Rectangle helpPanelRect() {
        const float pw = 0.52f * _SW, ph = 0.60f * _SH;
        return {(_SW - pw) / 2.f, (_SH - ph) / 2.f, pw, ph};
    }
    static Rectangle helpOkButton() {
        const Rectangle p = helpPanelRect();
        const float h = 0.055f * _SH < 44.f ? 44.f : 0.055f * _SH;
        return {p.x + (p.width - p.width * 0.5f) / 2.f,
                p.y + p.height - h - 18.f, p.width * 0.5f, h};
    }
    // Centered row of `count` buttons at height y. Single-button overlays
    // (handoff) get a wide button; multi-button rows split the width.
    static Rectangle overlayButton(int index, int count, float y) {
        const float w = (count == 1 ? 0.62f : 0.30f) * _SW;
        const float gap = 16.f;
        const float bh = 0.06f * _SH < 48.f ? 48.f : 0.06f * _SH;
        const float total = (float)count * w + (float)(count - 1) * gap;
        return {(_SW - total) / 2.f + (float)index * (w + gap), y, w, bh};
    }
    static Rectangle handoffButton() { return overlayButton(0, 1, 0.62f * _SH); }
    static Rectangle winButton(int index) { return overlayButton(index, 2, 0.64f * _SH); }
    // Chain banner: button docked at the banner's right edge (the banner rect
    // is computed in DuelPanels with room reserved for this button).
    static Rectangle bannerButton(float bx, float bw, float by, float bh) {
        const float w = 0.14f * _SW < 96.f ? 96.f : 0.14f * _SW;
        return {bx + bw - w - 6.f, by + 2.f, w, bh - 4.f};
    }
};

}  // namespace openjoey::ui
