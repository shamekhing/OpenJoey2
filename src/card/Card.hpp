#pragma once
#include "card/CardEffect.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace openjoey {

// ─── Enumerations ────────────────────────────────────────────────────────────

enum class CardType : uint8_t { Monster, Spell, Trap };

enum class Position : uint8_t { FaceUp, FaceDown };

enum class Location : uint8_t {
  Hand,
  Deck,
  ExtraDeck,
  Field,
  Graveyard,
  Banished,
  None
};

// ─── Card ────────────────────────────────────────────────────────────────────

// The one-and-only card class. A card's identity comes entirely from which
// Effects it subscribes to. No subclasses, ever.
// Filename stem in assets/cards. See download_card_images.py

struct Card {
  // Identity
  std::string name;
  uint32_t cardId = 0; // ygoproId, 0 = use cardId only
  uint32_t imageId = 0;
  std::string description;

  // Card classification
  CardType type = CardType::Monster;

  bool operator==(const Card &other) const {
    return this->cardId != 0 && this->cardId == other.cardId;
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

  // Placement
  Location location = Location::None;
  Position position = Position::FaceDown;

  bool isFaceUp() const {
    return location == Location::Field && position == Position::FaceUp;
  }

  // Turn-state flags
  bool setThisTurn = false;
  bool placedThisTurn = false;

  // Others
    // A Card's identity comes from the EffectIDs it subscribes to (see the comment
    // at the top of this struct).  CardEffect lives in card/ (layer 1), so this
    // keeps Card.hpp in layer 1 with no dependency on a later layer.
        std::vector<CardEffect> effects;

  std::vector<Card *> equippedCards; // non-owning equip attachments
  std::map<std::string, int> counters;

  // Convenience queries
  bool isMonster() const { return type == CardType::Monster; }
  bool isSpell() const { return type == CardType::Spell; }
  bool isTrap() const { return type == CardType::Trap; }
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
    return a.cardId < b.cardId;
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