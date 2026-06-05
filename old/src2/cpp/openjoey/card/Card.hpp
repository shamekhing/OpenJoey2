#pragma once
#include "Type.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace openjoey {

// ─── Card ────────────────────────────────────────────────────────────────────

// The one-and-only card class. A card's identity comes entirely from which
// Effects it subscribes to. No subclasses, ever.
// Filename stem in assets/cards. See download_card_images.py

struct Card {
  // Identity
  std::string name;
  uint32_t cardNumber = 0; // ygoproId, 0 = use cardNumber only
  uint32_t imageId = 0;
  std::string description;

  // Card classification
  enum_card type = enum_card::Monster;
  Location location = Location::None;
  Position position = Position::FaceUp;

  bool operator==(const Card &other) const {
    return this->cardNumber != 0 && this->cardNumber == other.cardNumber;
  }

  // Stats
  int atk = 0;
  int def = 0;
  int level = 0;

  std::string statLine() const {
    return this->isMonster() ? "Level " + std::to_string(this->level) +
                                   "  ATK " + std::to_string(this->atk) +
                                   "  DEF " + std::to_string(this->def)
                             : "";
  }
  std::string shortStat() const {
    return this->isMonster()
               ? "L" + std::to_string(this->level) + " " +
                     std::to_string(this->atk) + "/" + std::to_string(this->def)
               : "";
  }

  // Ownership
  int owner = -1;      // player index
  int controller = -1; // player index (may differ from owner)

  // Turn-state flags
  bool setThisTurn = false;
  bool placedThisTurn = false;

  // Others
  std::vector<Card *> equippedCards; // non-owning equip attachments
  std::map<std::string, int> counters;

  // Effect keys populated by EffectRegistry::bind() at startup — data only.
  std::vector<std::string> effectKeys;

  // Convenience queries
  bool isMonster() const { return type == enum_card::Monster; }
  bool isSpell() const { return type == enum_card::Spell; }
  bool isTrap() const { return type == enum_card::Trap; }
  std::string cardTypeTag() const {
    return this->isMonster() ? "[MON]"
           : this->isSpell() ? "[SPL]"
           : this->isTrap()  ? "[TRP]"
                             : "[UNK]";
  }

  // For sorting in deck editor
  static bool sortByCardType(const openjoey::Card &a, const openjoey::Card &b) {
    return a.type != b.type ? (int)a.type < (int)b.type : a.name < b.name;
  };
  static bool sortByName(const openjoey::Card &a, const openjoey::Card &b) {
    return a.name < b.name;
  };
  static bool sortById(const openjoey::Card &a, const openjoey::Card &b) {
    return a.cardNumber < b.cardNumber;
  }
  static bool sortByLevel(const openjoey::Card &a, const openjoey::Card &b) {
    return a.level != b.level ? a.level < b.level : a.name < b.name;
  }
  static bool sortByAtk(const openjoey::Card &a, const openjoey::Card &b) {
    return a.atk != b.atk ? a.atk < b.atk : a.name < b.name;
  }
  static bool sortByDef(const openjoey::Card &a, const openjoey::Card &b) {
    return a.def != b.def ? a.def < b.def : a.name < b.name;
  }
};

} // namespace openjoey
