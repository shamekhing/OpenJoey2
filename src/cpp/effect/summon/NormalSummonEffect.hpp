#pragma once
#include "effect/Effect.hpp"
#include "field/Zone.hpp"

namespace openjoey {

/**
 * Normal summons a level 1-4 monster from hand to an empty monster zone.
 *
 * handIndex and destZone are set by the action invoker before activation.
 */
class NormalSummonEffect : public Effect {
public:
  int handIndex = 0;  // index into turn player's hand zone
  int destZone  = -1; // -1 = auto (first empty)

  NormalSummonEffect() { spellSpeed = 1; }

  bool condition(IDuelContext &ctx) const override {
    // Phase check: Main1 = 2, Main2 = 4 (see etypes::phase)
    const int ph = ctx.phase();
    if (ph != 2 && ph != 4) return false; // not Main1 or Main2

    const int tp = ctx.turnPlayerIdx();
    if (ctx.hasNormalSummoned(tp)) return false;

    zone::ZoneStack_Hand &hand = ctx.field().handZones[tp];
    if (handIndex < 0 || handIndex >= hand.count()) return false;

    const Card *c = hand.peek(handIndex);
    if (!c || !c->isMonster()) return false;
    if (c->level > 4)         return false; // needs tribute
    if (ctx.field().firstEmptyMonsterZone(tp) < 0) return false;

    return true;
  }

  void cost(IDuelContext & /*ctx*/) override {}

  void activate(IDuelContext & /*ctx*/) override {}

  void resolve(IDuelContext &ctx) override {
    const int tp = ctx.turnPlayerIdx();
    zone::ZoneStack_Hand &hand = ctx.field().handZones[tp];

    if (handIndex < 0 || handIndex >= hand.count()) return;

    Card *c = hand.remove(hand.peek(handIndex));
    if (!c) return;

    const int zi = (destZone >= 0) ? destZone
                                   : ctx.field().firstEmptyMonsterZone(tp);
    if (zi < 0 || !ctx.field().monsterZones[tp][zi].isEmpty()) {
      // No room — return to hand
      hand.put(c);
      return;
    }

    c->location   = etypes::location::Field;
    c->controller = tp;
    c->position   = etypes::position::FaceUp;
    c->placedThisTurn = false;
    c->setThisTurn    = false;

    ctx.field().monsterZones[tp][zi].put(c);
    ctx.setNormalSummoned(tp, true);
  }
};

} // namespace openjoey
