#pragma once
#include <filesystem>

namespace openjoey {

struct ContentPaths {
  static std::filesystem::path cardsJson() {
    return std::filesystem::current_path() / "data" / "cards.json";
  }
  static std::filesystem::path banlistJson() {
    return std::filesystem::current_path() / "data" / "banlist.json";
  }
  static std::filesystem::path cardImgDir() {
    return std::filesystem::current_path() / "data" / "card_images";
  }
  static std::filesystem::path ygoprodeckUrl() {
    return "https : // images.ygoprodeck.com/images/cards/"; // or cards_small
  };
};
} // namespace openjoey
