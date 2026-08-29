#pragma once
#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "card/ui/CardImageCache.hpp"
#include "ui/platform/Settings.hpp"
#include <vector>

namespace openjoey::ui {

// Shared application state passed to every screen. Avoids per-screen
// duplicates of CardDatabase references and CardImageCache instances.
struct AppContext {
    openjoey::CardDatabase&      cardDb;
    std::vector<openjoey::Card>& selectedDeck;
    CardImageCache&              imageCache;
    Settings&                    settings;
};

} // namespace openjoey::ui
