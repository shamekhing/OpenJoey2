#pragma once
#include <algorithm>
#include <raylib.h>

namespace openjoey::ui {

// Reusable keyboard cursor for 1-D lists. Wrapping mode suits menus;
// clamping mode suits scrollable panels with a defined item count.
struct KeyboardNav {
    int cursor = 0;
    int count  = 0;

    void setCount(int n) {
        count  = n;
        cursor = std::min(cursor, std::max(0, n - 1));
    }

    // Wrapping — used for cyclic menus.
    void wrapNext() { if (count > 0) cursor = (cursor + 1) % count; }
    void wrapPrev() { if (count > 0) cursor = (cursor - 1 + count) % count; }

    // Clamping — used for scrollable card lists.
    void clampNext(int step = 1) { cursor = std::min(cursor + step, std::max(0, count - 1)); }
    void clampPrev(int step = 1) { cursor = std::max(cursor - step, 0); }

    // Consume KEY_UP / KEY_DOWN and apply wrapping navigation.
    // Returns true if a key was consumed.
    bool handleWrapKeys() {
        if (IsKeyPressed(KEY_DOWN)) { wrapNext(); return true; }
        if (IsKeyPressed(KEY_UP))   { wrapPrev(); return true; }
        return false;
    }

    // Consume KEY_UP / KEY_DOWN with optional PAGE_UP / PAGE_DOWN.
    bool handleClampKeys(int pageStep = 10) {
        bool consumed = false;
        if (IsKeyPressed(KEY_DOWN))      { clampNext();         consumed = true; }
        if (IsKeyPressed(KEY_UP))        { clampPrev();         consumed = true; }
        if (IsKeyPressed(KEY_PAGE_DOWN)) { clampNext(pageStep); consumed = true; }
        if (IsKeyPressed(KEY_PAGE_UP))   { clampPrev(pageStep); consumed = true; }
        return consumed;
    }
};

} // namespace openjoey::ui
