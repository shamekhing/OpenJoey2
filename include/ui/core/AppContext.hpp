#pragma once
#include <ui/cards/CardImageCache.hpp>
#include <vector>

#include "Config.hpp"
#include "cards/Card.hpp"
#include "cards/CardDatabase.hpp"

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

// Shared application state passed to every screen. Avoids per-screen
// duplicates of CardDatabase references and CardImageCache instances.
struct AppContext {
    openjoey::cards::CardDatabase& cardDb;
    std::vector<openjoey::cards::Card>& selectedDeck;
    CardImageCache& imageCache;
    Config& settings;
};

}  // namespace openjoey::ui
