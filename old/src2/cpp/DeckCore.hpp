#pragma once
#include "CardRecord.hpp"
#include <cstddef>
#include <cstdint>
#include <vector>

namespace openjoey2 {

struct DeckStats {
  int total = 0;
  int monsters = 0;
  int spells = 0;
  int traps = 0;
};

class DeckCore {
public:
  static constexpr int kMinDeckSize = 40;
  static constexpr int kMaxDeckSize = 60;
  static constexpr int kMaxCopies = 3;

  bool add(const CardRecord &card);
  bool removeAt(size_t index);
  void clear();

  int count() const;
  int countCopies(uint32_t id) const;
  bool canDuel() const;
  const CardRecord *at(size_t index) const;
  DeckStats stats() const;

private:
  std::vector<CardRecord> cards_;
};

} // namespace openjoey2
