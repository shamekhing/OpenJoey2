#pragma once
#include <raylib.h>

#include <algorithm>
#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "ui/cards/CardImageCache.hpp"
#include "ui/cards/CardList.hpp"
#include "ui/cards/TextFit.hpp"
#include "ui/cards/Thumbnail.hpp"
#include "ui/widgets/StyleSheet.hpp"

// Scrollable 4-column card thumbnail grid. Replaces the old Grid widget.
namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

struct CardGrid {
    static int ColCount();

};

}  // namespace openjoey::ui
