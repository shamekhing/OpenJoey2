#pragma once
#include <raylib.h>

#include <functional>
#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "ui/cards/CardImageCache.hpp"
#include "ui/cards/TextFit.hpp"
#include "ui/cards/Thumbnail.hpp"
#include "ui/widgets/StyleSheet.hpp"

// Replaces the old List + ListItem pair. CardList renders a scrollable,
// cursor-tracked list of card rows with thumbnail, type tag, stat line,
// copy count, and selection border — all in one self-contained widget.
namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

struct CardList {
    // Single card row renderer (was ListItem).
    static void DrawItem(const openjoey::cards::Card& card, CardImageCache& cache, int x, int y, int w, bool selected, int copies, int maxCopies);

    // Full scrollable list (was List::Draw).
    static void Draw(const std::vector<const openjoey::cards::Card*>& cards, CardImageCache& cache, int x, int y, int w, int h, int cursor, bool focused, int maxCopies, std::function<int(uint32_t)> countFn);

    static Color cardTypeColor(const openjoey::cards::Card& c);
};

}  // namespace openjoey::ui
