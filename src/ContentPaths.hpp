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
  static std::string ygoprodeckUrl() {
    return "https://images.ygoprodeck.com/images/cards/";
  }
  static std::string ygoprodeckUrlSmall() {
    return "https://images.ygoprodeck.com/images/cards_small/";
  }
  static std::filesystem::path cardBackImg() {
    return std::filesystem::current_path() / "data" / "card_back.png";
  }
  static std::filesystem::path duelFieldBgImg() {
    return std::filesystem::current_path() / "data" / "assets" / "duel_field_background.png";
  }
};
} // namespace openjoey
