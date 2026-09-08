#pragma once
// ── TouchInput — long-press detection for the card detail overlay ────────────
// Tracks the current mouse/touch press across frames. Touch on web arrives as
// synthesized mouse input (raylib maps touch to mouse), so one tracker covers
// finger, pen and mouse everywhere.
#include <raylib.h>

namespace openjoey::ui {

struct PressTracker {
    bool active = false;
    Vector2 start{};
    double t0 = 0.0;

    // Call once per frame BEFORE anything consumes the click.
    void update() {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            active = true;
            start = GetMousePosition();
            t0 = GetTime();
        } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
            active = false;
        }
    }

    // True while the press has been held >= threshold seconds without moving
    // more than slopPx (a drag is a scroll/aim, not a press-and-hold).
    bool held(float thresholdSec = 0.35f, float slopPx = 14.f) const {
        if (!active || !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) return false;
        if (GetTime() - t0 < (double)thresholdSec) return false;
        const Vector2 d = GetMousePosition();
        const float dx = d.x - start.x, dy = d.y - start.y;
        return dx * dx + dy * dy <= slopPx * slopPx;
    }
};

}  // namespace openjoey::ui
