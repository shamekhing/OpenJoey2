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
  std::string description;
  uint32_t id = 0;
  uint32_t imageNumber = 0;
  etypes::card type = etypes::card::Monster;

  // Stats
  int atk = 0;
  int def = 0;
  int level = 0;

  // Ownership
  int owner = -1;      // player index
  int controller = -1; // player index (may differ from owner)
  etypes::location location = etypes::location::None;
  etypes::position position = etypes::position::FaceDown;

  // Turn-state flags
  bool setThisTurn = false;
  bool placedThisTurn = false;

  bool operator==(const Card &other) const {
    return this->id != 0 && this->id == other.id;
  }

  std::string const getStats(bool isShort = false) const {
    if (this->isMonster()) {
      auto atk = std::to_string(this->atk);
      auto def = std::to_string(this->def);
      auto lvl = std::to_string(this->level);
      // build and return appropriately-formed stat line
      return isShort ? std::string("L") + lvl + " " + atk + "/" + def
                     : std::string("Level ") + lvl + "  ATK " + atk + "  DEF " +
                           def;
    }
    return "";
  }

  bool isMonster() const { return type == etypes::card::Monster; }
  bool isSpell() const { return type == etypes::card::Spell; }
  bool isTrap() const { return type == etypes::card::Trap; }

  std::string getTypeTag() const {
    return this->isMonster() ? "[MON]"
           : this->isSpell() ? "[SPL]"
           : this->isTrap()  ? "[TRP]"
                             : "[UNK]";
  }
  // Others
  std::vector<Card *> equippedCards; // non-owning equip attachments
  std::map<std::string, int> counters;

  // Effect keys populated by EffectRegistry::bind() at startup — data only.
  std::vector<std::string> effectKeys;

  // Convenience queries

  template <typename Member> static auto sortBy(Member member) {
    return [member](const Card &c1, const Card &c2) {
      return c1.*member != c2.*member ? c1.*member < c2.*member
                                      : c1.name < c2.name;
    };
  }
};

} // namespace openjoey
