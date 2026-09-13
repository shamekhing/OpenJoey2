#include "ui/cards/Thumbnail.hpp"

namespace openjoey::ui {

void Thumbnail::Draw(const openjoey::cards::Card& card, CardImageCache& cache, int x, int y, int w, int h, Color typeCol) {
    const Texture2D* tex = cache.Get(card);
    Rectangle dst = {(float)x, (float)y, (float)w, (float)h};
    if (tex && tex->id != 0) DrawTexturePro(*tex, {0, 0, (float)tex->width, (float)tex->height}, dst, {0, 0}, 0, WHITE);
    else DrawRectangle(x, y, w, h, Color{typeCol.r, typeCol.g, typeCol.b, CARD_TYPE_ALPHA});
    DrawRectangleLines(x, y, w, h, typeCol);
}

}  // namespace openjoey::ui
