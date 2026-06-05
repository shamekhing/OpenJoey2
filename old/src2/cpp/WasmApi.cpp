#include "DeckCore.hpp"
#include "openjoey/duel/DuelCore.hpp"
#include "openjoey/effect/EffectFactory.hpp"
#include <cstdint>
#include <string>

using openjoey2::CardKind;
using openjoey2::CardRecord;
using openjoey2::DeckCore;
using openjoey2::DeckStats;

namespace {

CardKind toKind(int kind) {
  if (kind == 1)
    return CardKind::Spell;
  if (kind == 2)
    return CardKind::Trap;
  return CardKind::Monster;
}

openjoey::enum_card toNativeKind(int kind) {
  if (kind == 1)
    return openjoey::enum_card::Spell;
  if (kind == 2)
    return openjoey::enum_card::Trap;
  return openjoey::enum_card::Monster;
}

openjoey::Card makeNativeCard(uint32_t id, uint32_t imageId, int kind, int atk,
                              int def, int level) {
  openjoey::Card card;
  card.cardNumber = id;
  card.imageId = imageId;
  card.type = toNativeKind(kind);
  card.atk = atk;
  card.def = def;
  card.level = level;
  card.name = "Card " + std::to_string(id);
  card.location = openjoey::Location::Deck;
  return card;
}

struct OjGame {
  openjoey::EffectFactory factory;
  openjoey::game::DuelCore duel;
  std::array<std::vector<openjoey::Card>, 2> stagedDecks;

  OjGame() : factory(), duel(nullptr, &factory) {}
};

bool validPlayer(int player) { return player >= 0 && player < 2; }

} // namespace

extern "C" {

DeckCore *oj_deck_new() { return new DeckCore(); }

void oj_deck_free(DeckCore *deck) { delete deck; }

bool oj_deck_add(DeckCore *deck, uint32_t id, uint32_t imageId, int kind,
                 int atk, int def, int level) {
  if (!deck)
    return false;
  CardRecord card;
  card.id = id;
  card.imageId = imageId;
  card.kind = toKind(kind);
  card.atk = static_cast<int16_t>(atk);
  card.def = static_cast<int16_t>(def);
  card.level = static_cast<uint8_t>(level < 0 ? 0 : level);
  return deck->add(card);
}

bool oj_deck_remove_at(DeckCore *deck, int index) {
  if (!deck || index < 0)
    return false;
  return deck->removeAt(static_cast<size_t>(index));
}

void oj_deck_clear(DeckCore *deck) {
  if (deck)
    deck->clear();
}

int oj_deck_count(const DeckCore *deck) {
  return deck ? deck->count() : 0;
}

int oj_deck_count_copies(const DeckCore *deck, uint32_t id) {
  return deck ? deck->countCopies(id) : 0;
}

bool oj_deck_can_duel(const DeckCore *deck) {
  return deck ? deck->canDuel() : false;
}

uint32_t oj_deck_card_id(const DeckCore *deck, int index) {
  if (!deck || index < 0)
    return 0;
  const CardRecord *card = deck->at(static_cast<size_t>(index));
  return card ? card->id : 0;
}

int oj_deck_stats_total(const DeckCore *deck) {
  return deck ? deck->stats().total : 0;
}

int oj_deck_stats_monsters(const DeckCore *deck) {
  return deck ? deck->stats().monsters : 0;
}

int oj_deck_stats_spells(const DeckCore *deck) {
  return deck ? deck->stats().spells : 0;
}

int oj_deck_stats_traps(const DeckCore *deck) {
  return deck ? deck->stats().traps : 0;
}

OjGame *oj_game_new() { return new OjGame(); }

void oj_game_free(OjGame *game) { delete game; }

void oj_game_clear_decks(OjGame *game) {
  if (!game)
    return;
  game->stagedDecks[0].clear();
  game->stagedDecks[1].clear();
}

bool oj_game_add_deck_card(OjGame *game, int player, uint32_t id,
                           uint32_t imageId, int kind, int atk, int def,
                           int level) {
  if (!game || !validPlayer(player))
    return false;
  auto &deck = game->stagedDecks[player];
  if (deck.size() >= 60)
    return false;
  deck.push_back(makeNativeCard(id, imageId, kind, atk, def, level));
  return true;
}

bool oj_game_start(OjGame *game) {
  if (!game)
    return false;
  game->duel = openjoey::game::DuelCore(nullptr, &game->factory);
  game->duel.loadDeckFromCards(0, game->stagedDecks[0]);
  game->duel.loadDeckFromCards(1, game->stagedDecks[1]);
  game->duel.startDuel();
  return true;
}

int oj_game_turn_player(const OjGame *game) {
  return game ? game->duel.turnPlayerIdx() : 0;
}

int oj_game_phase(const OjGame *game) {
  return game ? game->duel.phase() : 0;
}

int oj_game_life_points(const OjGame *game, int player) {
  return game && validPlayer(player) ? game->duel.lifePoints(player) : 0;
}

int oj_game_winner(const OjGame *game) { return game ? game->duel.winner() : -2; }

int oj_game_deck_count(const OjGame *game, int player) {
  return game && validPlayer(player)
             ? game->duel.field().deckZones[player].count()
             : 0;
}

int oj_game_hand_count(const OjGame *game, int player) {
  return game && validPlayer(player)
             ? game->duel.field().handZones[player].count()
             : 0;
}

int oj_game_grave_count(const OjGame *game, int player) {
  return game && validPlayer(player)
             ? game->duel.field().graveyardZones[player].count()
             : 0;
}

int oj_game_banished_count(const OjGame *game, int player) {
  return game && validPlayer(player)
             ? game->duel.field().banishedZones[player].count()
             : 0;
}

uint32_t oj_game_hand_card_id(const OjGame *game, int player, int index) {
  if (!game || !validPlayer(player))
    return 0;
  const openjoey::Card *card = game->duel.field().handZones[player].peek(index);
  return card ? card->cardNumber : 0;
}

uint32_t oj_game_monster_zone_id(const OjGame *game, int player, int zone) {
  if (!game || !validPlayer(player) || zone < 0 || zone >= 5)
    return 0;
  const openjoey::Card *card = game->duel.field().monsterZones[player][zone].peek();
  return card ? card->cardNumber : 0;
}

uint32_t oj_game_spell_zone_id(const OjGame *game, int player, int zone) {
  if (!game || !validPlayer(player) || zone < 0 || zone >= 5)
    return 0;
  const openjoey::Card *card = game->duel.field().spellTrapZones[player][zone].peek();
  return card ? card->cardNumber : 0;
}

uint32_t oj_game_extra_monster_zone_id(const OjGame *game, int zone) {
  if (!game || zone < 0 || zone >= 2)
    return 0;
  const openjoey::Card *card = game->duel.field().extraMonsterZones[zone].peek();
  return card ? card->cardNumber : 0;
}

bool oj_game_draw(OjGame *game, int player) {
  if (!game || !validPlayer(player))
    return false;
  return game->duel.field().deckZones[player].draw(game->duel.field().handZones[player]);
}

bool oj_game_play_hand_at(OjGame *game, int player, int handIndex) {
  if (!game || !validPlayer(player))
    return false;
  auto &field = game->duel.field();
  openjoey::Card *card = field.handZones[player].peek(handIndex);
  if (!card)
    return false;
  const int zone = card->isMonster() ? field.firstEmptyMonsterZone(player)
                                     : field.firstEmptySpellTrapZone(player);
  if (zone < 0)
    return false;
  field.handZones[player].remove(card);
  card->location = openjoey::Location::Field;
  return card->isMonster() ? field.monsterZones[player][zone].put(card)
                           : field.spellTrapZones[player][zone].put(card);
}

bool oj_game_send_monster_to_grave(OjGame *game, int player, int zone) {
  if (!game || !validPlayer(player) || zone < 0 || zone >= 5)
    return false;
  auto &field = game->duel.field();
  openjoey::Card *card = field.monsterZones[player][zone].remove();
  if (!card)
    return false;
  card->location = openjoey::Location::Graveyard;
  return field.graveyardZones[player].put(card);
}

int oj_game_advance_phase(OjGame *game) {
  return game ? static_cast<int>(game->duel.advancePhase()) : 0;
}

}
