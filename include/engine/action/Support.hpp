#pragma once
#include "engine/action/Chains.hpp"
#include "engine/action/Moves.hpp"
#include "engine/action/State.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

inline ActionResult EquipCard(Duel &d, Card *equip, Card *monster) {
    if (!equip || !monster)
        return ActionResult::Fail("equip needs an equip card and a monster.");
    auto [z, p] = d.field.findCard(equip);
    if (!z || z->type() != zone::ZoneType::SpellTrap)
        return ActionResult::Fail(
            "the equip card must be set/activated in a spell/trap zone.");
    zone::Zone_Monster *mz = d.field.monsterZoneOf(monster);
    if (!mz || monster->state.controller != d.turnPlayer)
        return ActionResult::Fail("equip target must be your monster on the field.");
    if (!EquipAttach(equip, monster))
        return ActionResult::Fail("already equipped to that monster.");
    return ActionResult::Ok(equip->name + " equips " + monster->name + " (ATK +" +
                            std::to_string(equip->state.bonusAtk) + ").");
}
inline ActionResult UnequipCard(Duel &d, Card *equip) {
    (void)d;
    if (!equip || !EquipDetach(equip))
        return ActionResult::Fail("that card equips nothing.");
    return ActionResult::Ok(equip->name + " unequipped.");
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
inline ActionResult ResolveStandby(Duel &d) {
    if (d.turn.phase != Phase::Standby)
        return ActionResult::Fail("standby triggers resolve in the Standby Phase.");
    bool pushed = false;
    for (auto &mz : d.field.monsterZones[d.turnPlayer])
        if (Card *c = mz.peek())
            if (mz.isVisible())
                for (const auto &e : classicEffectsFor(c->name))
                    if (e.timing == EffectType::Trigger && e.id != ActionId::None) {
                        d.chain.push(e, c->state.controller);
                        pushed = true;
                    }
    if (!pushed) return ActionResult::Ok("no standby triggers.");
    if (d.config.chainResponseWindow)
        return ActionResult::Ok("standby triggers wait on the chain.");
    return ResolveChain(d);
}

}  // namespace openjoey::engine::action
