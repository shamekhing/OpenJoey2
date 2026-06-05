#include "DeckCore.hpp"
#include <iostream>

int main() {
  openjoey2::DeckCore deck;
  openjoey2::CardRecord monster{23771716, 23771716,
                                openjoey2::CardKind::Monster, 1800, 800, 4};
  for (int i = 0; i < 3; ++i) {
    if (!deck.add(monster))
      return 1;
  }
  if (deck.add(monster))
    return 2;
  std::cout << "deck smoke ok: " << deck.count() << " cards\n";
  return 0;
}
