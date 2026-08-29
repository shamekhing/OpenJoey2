#pragma once
#include "card/Card.hpp"
#include "card/ui/CardImageCache.hpp"
#include "ui/StyleSheet.hpp"
#include <raylib.h>

namespace openjoey::ui {

class Thumbnail {
public:
    static void Draw(const openjoey::Card& card, CardImageCache& cache,
                     int x, int y, int w, int h, Color typeCol) {
        const Texture2D* tex = cache.Get(card);
        Rectangle dst = {(float)x, (float)y, (float)w, (float)h};
        if (tex && tex->id != 0)
            DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height},
                           dst, {0, 0}, 0, WHITE);
        else
            DrawRectangle(x, y, w, h, Color{typeCol.r, typeCol.g, typeCol.b, CARD_TYPE_ALPHA});
        DrawRectangleLines(x, y, w, h, typeCol);
    }
};

} // namespace openjoey::ui
