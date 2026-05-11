#pragma once
#include <raylib.h>

// Divides a rectangle into N horizontal or vertical slices by percentage.
// Used to compute panel bounds without hardcoding pixel math in screens.
namespace openjoey::ui {

struct SplitView {
    // Horizontal split: returns the rect for slice [index] given an array of
    // width percentages (must sum to 100). The remainder goes to the last slice.
    static Rectangle hSlice(Rectangle total, const int* pcts, int count, int index) {
        float x = total.x;
        for (int i = 0; i < index; ++i)
            x += total.width * pcts[i] / 100.f;
        float w = (index == count - 1)
            ? (total.x + total.width - x)
            : total.width * pcts[index] / 100.f;
        return {x, total.y, w, total.height};
    }

    // Vertical split: returns the rect for slice [index].
    static Rectangle vSlice(Rectangle total, const int* pcts, int count, int index) {
        float y = total.y;
        for (int i = 0; i < index; ++i)
            y += total.height * pcts[i] / 100.f;
        float h = (index == count - 1)
            ? (total.y + total.height - y)
            : total.height * pcts[index] / 100.f;
        return {total.x, y, total.width, h};
    }
};

} // namespace openjoey::ui
