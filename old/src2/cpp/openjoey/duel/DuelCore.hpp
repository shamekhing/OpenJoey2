#pragma once
#include "card/ICardRepository.hpp"
#include "effect/Effect.hpp"
#include "effect/EffectFactory.hpp"
#include "effect/IDuelContext.hpp"
#include "duel/Chain.hpp"
#include "duel/PhaseManager.hpp"
#include "duel/ZoneEffectManager.hpp"
#include "zone/Field.hpp"
#include <array>
#include <iostream>
#include <vector>

namespace openjoey::game {

// ─── DuelCore ─────────────────────────────────────────────────────────────────
// Central duel state. Owns the field, card pools, chain, phase manager, and
// zone-effect manager. Implements IDuelContext so effects can call back into it.
//
// Card ownership rule: pool_[p] holds all cards for player p by value.
// Zone::put() receives raw pointers into the pool — the pool is reserved
// before dealing so it never reallocates (stable Card* for the whole duel).

class DuelCore : public openjoey::IDuelContext {
public:
  // ── Setup ────────────────────────────────────────────────────────────────

  explicit DuelCore(ICardRepository *repo = nullptr,
                    EffectFactory   *factory = nullptr)
      : repo_(repo), factory_(factory) {
    pool_[0].reserve(60);
    pool_[1].reserve(60);
    normalSummoned_[0] = normalSummoned_[1] = false;
  }

  // Load a deck for player p from the repository by card ID list.
  // Call before startDuel().
  void loadDeck(int player, const std::vector<uint32_t> &ids) {
    if (player < 0 || player > 1) return;
    for (uint32_t id : ids) {
      if (!repo_) break;
      const Card *tmpl = repo_->getById(id);
      if (!tmpl) continue;
      if (pool_[player].size() >= 60) break;
      pool_[player].push_back(*tmpl);
      Card &c = pool_[player].back();
      c.owner      = player;
      c.controller = player;
      c.location   = Location::Deck;
    }
  }

  // Load a deck directly from a card vector (e.g. from DeckEditorScreen).
  void loadDeckFromCards(int player, const std::vector<openjoey::Card> &cards) {
    if (player < 0 || player > 1) return;
    pool_[player].clear();
    pool_[player].reserve(60);
    for (const Card &src : cards) {
      if (pool_[player].size() >= 60) break;
      pool_[player].push_back(src);
      Card &c = pool_[player].back();
      c.owner      = player;
      c.controller = player;
      c.location   = Location::Deck;
    }
  }

  // Wire all cards from pool into their deck zones, shuffle, deal opening hands.
  void startDuel() {
    field_ = zone::Field(); // reset
    for (int p = 0; p < 2; ++p) {
      for (Card &c : pool_[p]) {
        c.location = Location::Deck;
        field_.deckZones[p].put(&c);
      }
      field_.deckZones[p].shuffle();
    }
    // Deal 5 cards to each player
    for (int p = 0; p < 2; ++p)
      for (int i = 0; i < 5; ++i)
        field_.deckZones[p].draw(field_.handZones[p]);

    phase_.phase       = Phase::Draw;
    phase_.turnPlayer  = 0;
    phase_.turnNumber  = 1;
    phase_.isFirstTurn = true;
    chain_.clear();
    zem_.clear();
    winner_            = -2;
    rngState_          = 0xC0FFEEu;
    lifePoints_[0]     = lifePoints_[1] = 8000;
    normalSummoned_[0] = normalSummoned_[1] = false;
  }

  void setSeed(uint32_t seed) { rngState_ = seed; }

  // ── IDuelContext ──────────────────────────────────────────────────────────

  zone::Field       &field()       override { return field_; }
  const zone::Field &field() const           { return field_; }

  int turnPlayerIdx() const override { return phase_.turnPlayer; }
  int opponentIdx()   const override { return 1 - phase_.turnPlayer; }

  bool hasNormalSummoned(int player) const override {
    return (player >= 0 && player < 2) ? normalSummoned_[player] : false;
  }
  void setNormalSummoned(int player, bool v) override {
    if (player >= 0 && player < 2) normalSummoned_[player] = v;
  }

  void setWinner(int player) override { winner_ = player; }

  void pushTargetRequest(TargetRequest req) override {
    targetRequest_ = req;
  }
  const TargetRequest &targetRequest() const override { return targetRequest_; }
  void clearTargetRequest() override {
    targetRequest_ = TargetRequest{};
  }

  uint32_t nextRng() override {
    rngState_ ^= rngState_ << 13;
    rngState_ ^= rngState_ >> 17;
    rngState_ ^= rngState_ << 5;
    return rngState_;
  }

  int phase() const override { return static_cast<int>(phase_.phase); }

  void registerPersistentEffect(Effect *eff, zone::IZone *srcZone,
                                Card *srcCard) override {
    zem_.registerEffect(eff, srcZone, srcCard);
  }

  void negateNextChainLink() override { chain_.requestNegateNext(); }

  // ── Turn / Phase control ─────────────────────────────────────────────────

  Phase advancePhase() {
    if (phase_.phase == Phase::End)
      resetTurnFlags();
    auto p = phase_.advance();
    if (p == Phase::Draw)
      runDrawPhase();
    zem_.tick(*this);
    checkWinConditions();
    return p;
  }

  // ── Effect activation ────────────────────────────────────────────────────

  // Creates a fresh Effect from the factory, runs condition/cost/push/activate.
  // Returns false if condition fails, key unknown, or spell-speed rejected.
  bool activateEffect(const std::string &key, Card *sourceCard, int activatingPlayer) {
    if (!factory_) {
      std::cerr << "[DuelCore] no EffectFactory\n";
      return false;
    }
    auto eff = factory_->create(key);
    if (!eff) return false;

    eff->sourceCard = sourceCard;

    if (!eff->condition(*this)) return false;
    eff->cost(*this);

    Effect *rawPtr = eff.get();
    if (!chain_.push(std::move(eff), activatingPlayer, sourceCard)) return false;

    rawPtr->activate(*this);
    passCount_ = 0;
    return true;
  }

  // Pass priority. After two consecutive passes the chain resolves.
  void passPriority() {
    ++passCount_;
    if (passCount_ >= 2)
      resolveChain();
  }

  // Resolve all chain links immediately.
  void resolveChain() {
    while (!chain_.isEmpty() && !hasPendingTarget())
      chain_.resolveNext(*this);
    if (chain_.isEmpty()) {
      chain_.clear();
      passCount_ = 0;
    }
    zem_.tick(*this);
    checkWinConditions();
  }

  bool hasPendingTarget() const {
    return targetRequest_.kind != TargetRequest::Kind::None &&
           !targetRequest_.fulfilled;
  }

  // ── Win conditions ───────────────────────────────────────────────────────

  void checkWinConditions() {
    for (int p = 0; p < 2; ++p) {
      if (lifePoints_[p] <= 0)
        winner_ = 1 - p;
    }
  }

  // ── State accessors ──────────────────────────────────────────────────────

  int  winner()     const { return winner_; }
  bool isDuelOver() const { return winner_ != -2; }

  const PhaseManager &phaseManager() const { return phase_; }

  int lifePoints(int player) const {
    return (player >= 0 && player < 2) ? lifePoints_[player] : 0;
  }
  void applyDamage(int player, int amount) {
    if (player < 0 || player > 1) return;
    lifePoints_[player] = std::max(0, lifePoints_[player] - amount);
    checkWinConditions();
  }
  void gainLP(int player, int amount) {
    if (player < 0 || player > 1) return;
    lifePoints_[player] += amount;
  }

  const Chain &chain() const { return chain_; }

  // Read-only access to the raw card pools (for UI test helpers).
  const std::array<std::vector<openjoey::Card>, 2> &pools() const { return pool_; }

private:
  zone::Field        field_;
  Chain              chain_;
  PhaseManager       phase_;
  ZoneEffectManager  zem_;

  ICardRepository *repo_    = nullptr;
  EffectFactory   *factory_ = nullptr;

  std::array<std::vector<Card>, 2> pool_; // owns all card instances
  int  winner_             = -2; // -2=ongoing, -1=draw, 0/1=player index
  int  passCount_          = 0;
  bool normalSummoned_[2]  = {false, false};
  int  lifePoints_[2]      = {8000, 8000};
  uint32_t rngState_       = 0xC0FFEEu;

  TargetRequest targetRequest_;

  void runDrawPhase() {
    const int p = phase_.turnPlayer;
    if (phase_.isFirstTurn && p == 0) return; // player 0 skips draw on turn 1
    if (field_.deckZones[p].isEmpty()) {
      winner_ = 1 - p; // cannot draw → lose
      return;
    }
    field_.deckZones[p].draw(field_.handZones[p]);
  }

  void resetTurnFlags() {
    normalSummoned_[0] = normalSummoned_[1] = false;
    // Reset per-card turn flags for cards on field
    for (int p = 0; p < 2; ++p) {
      for (int z = 0; z < 5; ++z) {
        if (Card *c = field_.monsterZones[p][z].peek()) {
          c->setThisTurn    = false;
          c->placedThisTurn = false;
        }
        if (Card *c = field_.spellTrapZones[p][z].peek()) {
          c->setThisTurn = false;
        }
      }
    }
  }
};

} // namespace openjoey::game
