#pragma once
#include <cstdint>

namespace openjoey::ui {

enum class AppScreen : uint8_t {
  MainMenu,
  DeckEditor,
  Duel,
  Testing,
  Settings
};

} // namespace openjoey::ui
