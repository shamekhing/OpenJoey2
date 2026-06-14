#pragma once
#include "Type.hpp"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace openjoey {

/**
 * Runtime card state used by the rules engine.
 *
 * JavaScript owns database parsing and presentation fields. C++ only keeps the
 * fields required to enforce duel/deck mechanics and expose state back through
 * the C ABI.
 */
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

  bool isMonster() const { return type == etypes::card::Monster; }
  bool isSpell() const { return type == etypes::card::Spell; }
  bool isTrap() const { return type == etypes::card::Trap; }

  // Attachments/counters are non-owning runtime relationships.
  std::vector<Card *> equippedCards; // non-owning equip attachments
  std::map<std::string, int> counters;

  // Effect keys supplied by adapters/data loaders.
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
