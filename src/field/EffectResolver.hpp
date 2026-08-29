#pragma once
#include "card/CardEffect.hpp"
#include "field/EffectsBuiltIn.hpp"
#include <string>

namespace openjoey {

// ── Effect arguments ─────────────────────────────────────────────────────────
struct EffectArgs {
  int n = 1;                   // for n-card moves (draw / mill / discard)
  bool faceDown = false;       // for banish (face-down cost)
  int targetPlayer = -1;       // -1 => use activator
  Card *target = nullptr;      // the specific card an effect targets
};

// ── EffectResolver ───────────────────────────────────────────────────────────
// The single dispatch point that turns an EffectID into concrete zone moves.
// Auditing "where does each card go for effect X?" means reading this one
// function.  Adding a new effect = add a case below (+ optionally a helper in
// field/EffectsBuiltIn.hpp).  You do NOT touch card/ or zone/ to add one —
// the layering invariant (card -> zone -> field -> duel) is preserved.
struct EffectResolver {
  zone::Field &field;
  explicit EffectResolver(zone::Field &f) : field(f) {}

  std::string apply(EffectID id, int activator,
                    const EffectArgs &a = {}) const {
    int tp = (a.targetPlayer >= 0) ? a.targetPlayer : activator;
    switch (id) {
    case EffectID::Cost_Discard:
      return std::to_string(Move_DiscardToGY(field, tp, a.n)) + " card(s) discarded as cost.";
    case EffectID::Cost_BanishCost:
      return Move_Banish(field, a.target, /*faceDown=*/true)
                 ? "banished (face-down) as cost."
                 : "banish-cost: no target in a zone.";
    case EffectID::Cost_Tribute:
      return Move_DestroyToGY(field, a.target)
                 ? "tributed -> Graveyard."
                 : "tribute: no target in a zone.";
    case EffectID::Cost_PayLP:
      return "cost: pay " + std::to_string(a.n) + " LP.";
    case EffectID::Move_Draw:
      return std::to_string(Move_Draw(field, tp, a.n)) + " card(s) drawn.";
    case EffectID::Move_MillToGY:
      return std::to_string(Move_MillToGY(field, tp, a.n)) + " card(s) milled.";
    case EffectID::Move_DiscardToGY:
      return std::to_string(Move_DiscardToGY(field, tp, a.n)) + " card(s) discarded.";
    case EffectID::Move_DestroyToGY:
    case EffectID::Move_SendToGY:
      return Move_DestroyToGY(field, a.target)
                 ? "destroyed -> Graveyard."
                 : "destroy: no target in a zone.";
    case EffectID::Move_Banish:
      return Move_Banish(field, a.target, a.faceDown)
                 ? "banished."
                 : "banish: no target in a zone.";
    case EffectID::Move_ReturnHand:
      return Move_ReturnHand(field, a.target)
                 ? "returned to hand."
                 : "return: no target in a zone.";
    case EffectID::Move_ReturnDeck:
      return Move_ReturnDeck(field, a.target)
                 ? "returned to deck."
                 : "return: no target in a zone.";
    case EffectID::Summon_Normal:
      return Summon_Normal(field, a.target, tp) ? "normal summon OK."
                                                : "normal summon failed (no zone).";
    case EffectID::Summon_Set:
      return Summon_Set(field, a.target, tp) ? "set OK."
                                             : "set failed (no zone).";
    case EffectID::Summon_Special:
      return Summon_Special(field, a.target, tp) ? "special summon OK."
                                                 : "special summon failed (no zone).";
    case EffectID::Pos_Flip:
      return Pos_Flip(field, a.target) ? "flipped face-up."
                                       : "flip: target is not a set monster.";
    case EffectID::LP_Damage:
      return "deal " + std::to_string(a.n) + " damage.";
    case EffectID::LP_Gain:
      return "gain " + std::to_string(a.n) + " LP.";
    case EffectID::NegateActivation:
    case EffectID::NegateEffect:
    case EffectID::Pos_ChangeAToDef:
    case EffectID::Pos_ChangeDefToAtk:
    case EffectID::Summon_Flip:
    case EffectID::Summon_Fusion:
    case EffectID::Summon_Synchro:
    case EffectID::Summon_Xyz:
    case EffectID::Summon_Ritual:
      return "TODO: effect not wired yet.";
    case EffectID::None:
      return "idle.";
    }
    return "no such effect.";
  }
};

} // namespace openjoey
