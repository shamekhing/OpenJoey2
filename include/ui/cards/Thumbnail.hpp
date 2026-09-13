#pragma once
#include <raylib.h>

#include "cards/Card.hpp"
#include "ui/cards/CardImageCache.hpp"
#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

class Thumbnail {
   public:
    static void Draw(const openjoey::cards::Card& card, CardImageCache& cache, int x, int y, int w, int h, Color typeCol);
};

}  // namespace openjoey::ui
