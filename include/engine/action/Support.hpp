#pragma once
#include "engine/action/State.hpp"
#include "engine/action/Moves.hpp"
#include "engine/action/Chains.hpp"

namespace openjoey::engine::action {

inline std::string EquipCard(Duel &d, Card *equip, Card *monster) {
  if (!equip || !monster) return "equip needs an equip card and a monster.";
  auto [z, p] = d.field.findCard(equip);
  if (!z || z->type() != zone::ZoneType::SpellTrap)
    return "the equip card must be set/activated in a spell/trap zone.";
  zone::Zone_Monster *mz = d.field.monsterZoneOf(monster);
  if (!mz || monster->state.controller != d.turnPlayer)
    return "equip target must be your monster on the field.";
  if (!EquipAttach(equip, monster))
    return "already equipped to that monster.";
  return equip->name + " equips " + monster->name + " (ATK +" +
         std::to_string(equip->state.bonusAtk) + ").";
}
inline std::string UnequipCard(Duel &d, Card *equip) {
  (void)d;
  if (!equip || !EquipDetach(equip)) return "that card equips nothing.";
  return equip->name + " unequipped.";
}
inline void PlaceCounterD(Duel &d, Card *c, const std::string &ctr, int n = 1) {
  (void)d;
  if (c) PlaceCounter(*c, ctr, n);
}
inline int RemoveCounterD(Duel &d, Card *c, const std::string &ctr, int n = 1) {
  (void)d;
  return c ? RemoveCounter(*c, ctr, n) : 0;
}
inline int SearchDeck(Duel &d, const std::function<bool(const Card &)> &pred) {
  return MoveSearchToHand(d.field, d.turnPlayer, pred);
}
inline std::vector<Card *> Excavate(Duel &d, int n) {
  return MoveExcavate(d.field, d.turnPlayer, n);
}
inline std::string ResolveStandby(Duel &d) {
  if (d.turn.phase != Phase::Standby)
    return "standby triggers resolve in the Standby Phase.";
  bool pushed = false;
  for (auto &mz : d.field.monsterZones[d.turnPlayer])
    if (Card *c = mz.peek())
      if (mz.isVisible())
        for (const auto &e : classicEffectsFor(c->name))
          if (e.timing == EffectType::Trigger && e.id != ActionId::None) {
            d.chain.push(e, c->state.controller);
            pushed = true;
          }
  if (!pushed) return "no standby triggers.";
  if (d.config.chainResponseWindow) return "standby triggers wait on the chain.";
  return ResolveChain(d);
}

} // namespace openjoey::engine::action
