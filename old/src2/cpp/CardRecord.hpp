#pragma once
#include <cstdint>

namespace openjoey2 {

enum class CardKind : uint8_t {
  Monster = 0,
  Spell = 1,
  Trap = 2,
};

struct CardRecord {
  uint32_t id = 0;
  uint32_t imageId = 0;
  CardKind kind = CardKind::Monster;
  int16_t atk = 0;
  int16_t def = 0;
  uint8_t level = 0;
};

} // namespace openjoey2
