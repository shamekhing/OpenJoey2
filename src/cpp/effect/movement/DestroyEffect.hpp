#pragma once
#include "effect/Effect.hpp"
#include "field/Zone.hpp"

namespace openjoey {

/**
 * Requests a target zone and sends the selected card to its owner's graveyard.
 */
class DestroyEffect : public Effect {
public:
  enum class Scope {
    OppMonster,   // opponent's monster zone
    AnyMonster,   // any monster zone
    AnySpellTrap, // any spell/trap zone
    OppSpellTrap, // opponent's spell/trap zone
  };

  explicit DestroyEffect(Scope scope = Scope::OppMonster)
      : scope_(scope) {
    spellSpeed = 1;
  }

  bool condition(IDuelContext &ctx) const override {
    const int opp = ctx.opponentIdx();
    zone::Field &f = const_cast<IDuelContext &>(ctx).field(); // IDuelContext::field() not yet const-overloaded
    switch (scope_) {
    case Scope::OppMonster:
      return f.countMonsters(opp) > 0;
    case Scope::AnyMonster:
      return f.countMonsters(0) + f.countMonsters(1) > 0;
    case Scope::OppSpellTrap:
      for (int i = 0; i < 5; ++i)
        if (!f.spellTrapZones[opp][i].isEmpty()) return true;
      return false;
    case Scope::AnySpellTrap:
      for (int p = 0; p < 2; ++p)
        for (int i = 0; i < 5; ++i)
          if (!f.spellTrapZones[p][i].isEmpty()) return true;
      return false;
    }
    return false;
  }

  void cost(IDuelContext & /*ctx*/) override {}

  void activate(IDuelContext &ctx) override {
    TargetRequest req;
    req.sourcePlayer = ctx.turnPlayerIdx();
    switch (scope_) {
    case Scope::OppMonster:
      req.kind = TargetRequest::Kind::OppMonsterZone;
      break;
    case Scope::AnyMonster:
      req.kind = TargetRequest::Kind::AnyMonsterZone;
      break;
    case Scope::OppSpellTrap:
      req.kind = TargetRequest::Kind::OppSpellTrapZone;
      break;
    case Scope::AnySpellTrap:
      req.kind = TargetRequest::Kind::AnySpellTrapZone;
      break;
    }
    ctx.pushTargetRequest(req);
  }

  void resolve(IDuelContext &ctx) override {
    const TargetRequest &req = ctx.targetRequest();
    if (!req.fulfilled) return;

    zone::Field &f   = ctx.field();
    const int    tp  = ctx.turnPlayerIdx();
    const int    opp = ctx.opponentIdx();

    // Resolve target coordinates into the actual zone at resolution time.
    zone::IZone *srcZone = nullptr;

    if (req.kind == TargetRequest::Kind::OppMonsterZone ||
        req.kind == TargetRequest::Kind::AnyMonsterZone) {
      const int zp = (req.resolvedPlayer >= 0) ? req.resolvedPlayer : opp;
      const int zi = req.resolvedZone;
      if (zi >= 0 && zi < 5)
        srcZone = &f.monsterZones[zp][zi];
    } else {
      const int zp = (req.resolvedPlayer >= 0) ? req.resolvedPlayer : opp;
      const int zi = req.resolvedZone;
      if (zi >= 0 && zi < 5)
        srcZone = &f.spellTrapZones[zp][zi];
    }

    if (!srcZone || srcZone->isEmpty()) {
      ctx.clearTargetRequest();
      return;
    }

    Card *c = srcZone->remove();
    if (c) {
      c->location   = etypes::location::Graveyard;
      c->controller = c->owner;
      f.graveyardZones[c->owner].put(c);
    }
    ctx.clearTargetRequest();
    (void)tp;
  }

private:
  Scope scope_;
};

} // namespace openjoey
