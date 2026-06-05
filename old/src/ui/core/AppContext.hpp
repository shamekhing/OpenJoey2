#pragma once
#include "card/Card.hpp"
#include "card/ICardRepository.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include <vector>

namespace openjoey::ui {

// Shared application state passed to every screen.
struct AppContext {
    std::vector<openjoey::Card>& selectedDeck;
    CardImageCache&              imageCache;
    openjoey::ICardRepository*   cardRepo = nullptr;
};

} // namespace openjoey::ui
