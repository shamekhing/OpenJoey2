#include <cstdint>
#include <iostream>

struct DeckCore;
struct OjGame;

extern "C" {
DeckCore *oj_deck_new();
void oj_deck_free(DeckCore *deck);
bool oj_deck_add(DeckCore *deck, uint32_t id, uint32_t imageId, int kind,
                 int atk, int def, int level);
int oj_deck_count(const DeckCore *deck);

OjGame *oj_game_new();
void oj_game_free(OjGame *game);
bool oj_game_add_deck_card(OjGame *game, int player, uint32_t id,
                           uint32_t imageId, int kind, int atk, int def,
                           int level);
bool oj_game_start(OjGame *game);
int oj_game_hand_count(const OjGame *game, int player);
int oj_game_deck_count(const OjGame *game, int player);
int oj_game_phase(const OjGame *game);
int oj_game_turn_player(const OjGame *game);
int oj_game_advance_phase(OjGame *game);
}

int main() {
  DeckCore *deck = oj_deck_new();
  if (!deck) {
    std::cerr << "deck allocation failed\n";
    return 1;
  }

  if (!oj_deck_add(deck, 89631139, 89631139, 0, 2500, 2100, 7) ||
      oj_deck_count(deck) != 1) {
    std::cerr << "deck API smoke check failed\n";
    oj_deck_free(deck);
    return 1;
  }
  oj_deck_free(deck);

  OjGame *game = oj_game_new();
  if (!game) {
    std::cerr << "game allocation failed\n";
    return 1;
  }

  for (int player = 0; player < 2; ++player) {
    for (int i = 0; i < 40; ++i) {
      const uint32_t id = static_cast<uint32_t>(10000000 + player * 100 + i);
      if (!oj_game_add_deck_card(game, player, id, id, 0, 1000 + i, 1000, 4)) {
        std::cerr << "game deck staging failed\n";
        oj_game_free(game);
        return 1;
      }
    }
  }

  if (!oj_game_start(game) || oj_game_hand_count(game, 0) != 5 ||
      oj_game_hand_count(game, 1) != 5 || oj_game_deck_count(game, 0) != 35 ||
      oj_game_deck_count(game, 1) != 35) {
    std::cerr << "game API smoke check failed\n";
    oj_game_free(game);
    return 1;
  }

  if (oj_game_phase(game) != 0 || oj_game_turn_player(game) != 0 ||
      oj_game_advance_phase(game) != 1 || oj_game_advance_phase(game) != 2 ||
      oj_game_advance_phase(game) != 4 || oj_game_advance_phase(game) != 5 ||
      oj_game_advance_phase(game) != 0 || oj_game_turn_player(game) != 1 ||
      oj_game_hand_count(game, 1) != 6 || oj_game_deck_count(game, 1) != 34) {
    std::cerr << "game phase API smoke check failed\n";
    oj_game_free(game);
    return 1;
  }

  oj_game_free(game);
  std::cout << "OpenJoey2 native smoke check passed\n";
  return 0;
}
