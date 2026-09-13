#pragma once
#include <raylib.h>

#include <string>
#include <vector>

#include "cards/Card.hpp"
#include "ui/widgets/ProgressBar.hpp"
#include "ui/widgets/StyleSheet.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

class DeckStats {
   public:
    static void Draw(const std::vector<openjoey::cards::Card>& deck, int minSize, int x, int y, int w);
};

}  // namespace openjoey::ui
