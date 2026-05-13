#pragma once
#include "card/Card.hpp"
#include "card/CardDatabase.hpp"
#include "card/ICardRepository.hpp"
#include "ui/renderer/CardImageCache.hpp"
#include <vector>

namespace openjoey::ui {

// Shared application state passed to every screen.
struct AppContext {
    openjoey::CardDatabase&      cardDb;        // legacy — kept for DeckEditorScreen
    std::vector<openjoey::Card>& selectedDeck;
    CardImageCache&              imageCache;
    openjoey::ICardRepository*   cardRepo = nullptr; // repository layer (set by App)
};

} // namespace openjoey::ui
