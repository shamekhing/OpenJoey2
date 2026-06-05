#include "DeckCore.hpp"

namespace openjoey2 {

bool DeckCore::add(const CardRecord &card) {
  if (cards_.size() >= kMaxDeckSize)
    return false;
  if (countCopies(card.id) >= kMaxCopies)
    return false;
  cards_.push_back(card);
  return true;
}

bool DeckCore::removeAt(size_t index) {
  if (index >= cards_.size())
    return false;
  cards_.erase(cards_.begin() + static_cast<std::ptrdiff_t>(index));
  return true;
}

void DeckCore::clear() { cards_.clear(); }

int DeckCore::count() const { return static_cast<int>(cards_.size()); }

int DeckCore::countCopies(uint32_t id) const {
  int copies = 0;
  for (const CardRecord &card : cards_) {
    if (card.id == id)
      ++copies;
  }
  return copies;
}

bool DeckCore::canDuel() const {
  return cards_.size() >= kMinDeckSize && cards_.size() <= kMaxDeckSize;
}

const CardRecord *DeckCore::at(size_t index) const {
  if (index >= cards_.size())
    return nullptr;
  return &cards_[index];
}

DeckStats DeckCore::stats() const {
  DeckStats out;
  out.total = static_cast<int>(cards_.size());
  for (const CardRecord &card : cards_) {
    switch (card.kind) {
    case CardKind::Monster:
      ++out.monsters;
      break;
    case CardKind::Spell:
      ++out.spells;
      break;
    case CardKind::Trap:
      ++out.traps;
      break;
    }
  }
  return out;
}

} // namespace openjoey2
