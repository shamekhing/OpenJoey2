#pragma once
#include "effect/Effect.hpp"
#include "field/Zone.hpp"

namespace openjoey {

// ─── DrawEffect ───────────────────────────────────────────────────────────────
// Draws `count` cards from the turn player's deck to their hand.
// Ends the duel immediately if the deck runs out during the draw.

class DrawEffect : public Effect {
public:
  explicit DrawEffect(int count = 1) : count_(count) { spellSpeed = 1; }

  bool condition(IDuelContext &ctx) const override {
    const int p = ctx.turnPlayerIdx();
    return ctx.field().deckZones[p].count() >= count_;
  }

  void cost(IDuelContext & /*ctx*/) override {}

  void activate(IDuelContext & /*ctx*/) override {}

  void resolve(IDuelContext &ctx) override {
    const int p = ctx.turnPlayerIdx();
    zone::ZoneStack_Deck &deck = ctx.field().deckZones[p];
    zone::ZoneStack_Hand &hand = ctx.field().handZones[p];
    for (int i = 0; i < count_; ++i) {
      if (deck.isEmpty()) {
        ctx.setWinner(1 - p); // cannot draw → opponent wins
        return;
      }
      deck.draw(hand);
    }
  }

private:
  int count_;
};

} // namespace openjoey
