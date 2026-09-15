#pragma once
// act/Action - THE interpreter (the one dispatcher of the layer).
// `apply()` realizes one `Action{op, scope, amount}` against the mat through
// the Move.hpp primitives. Every path that runs an op - chain-link resolution,
// cost lists, (later) card specs - goes through here. There is no second
// dispatcher: Perform is gone, the chain resolver calls apply().
// Scope resolution (which cards an Action hits) also lives here, so
// "Op x Scope" semantics are readable in exactly one place.
#include <vector>

#include "action/ActionArgs.hpp"
#include "action/ActionResult.hpp"
#include "engine/action/Move.hpp"
#include "engine/action/Query.hpp"

namespace openjoey::engine::action {

using openjoey::Action;
using openjoey::ActionId;
using openjoey::Scope;
using cards::Card;

// The execution context of one realized Action.
struct ActionCtx {
    Duel &d;
    int activator;      // player realizing the action
    ActionArgs args;    // target / source / n / materials / targetPlayer override
};

// Scope -> concrete card list (the single home of scope semantics).
inline std::vector<Card *> resolveScope(const Action &a, const ActionCtx &c) {
    std::vector<Card *> out;
    const int me = c.activator;
    switch (a.scope) {
        case Scope::Target:
            if (c.args.target) out.push_back(c.args.target);
            break;
        case Scope::OppMonsters:
        case Scope::PerOppMonster:
            for (auto &mz : c.d.field.monsterZones[1 - me])
                if (Card *card = mz.peek()) out.push_back(card);
            break;
        case Scope::AllMonsters:
            for (int p = 0; p < zone::Field::PLAYERS; ++p)
                for (auto &mz : c.d.field.monsterZones[p])
                    if (Card *card = mz.peek()) out.push_back(card);
            break;
        case Scope::AllSpellsTraps:
            for (int p = 0; p < zone::Field::PLAYERS; ++p)
                for (auto &st : c.d.field.spellTrapZones[p])
                    if (Card *card = st.peek()) out.push_back(card);
            break;
        case Scope::OppAttackPos:
            for (auto &mz : c.d.field.monsterZones[1 - me])
                if (Card *card = mz.peek())
                    if (mz.isVisible() && mz.orientation() == zone::Orientation::Vertical)
                        out.push_back(card);
            break;
        case Scope::Activator:
        case Scope::Opponent:
        case Scope::None:
            break;  // player-scoped ops handle me/victim themselves
    }
    return out;
}

// THE dispatcher: one Action -> mat primitives. Never a silent success.
inline ActionResult apply(const Action &a, ActionCtx &c) {
    Duel &d = c.d;
    const int me = c.activator;
    switch (a.op) {
        // 1. ACTIVATION COSTS
        case ActionId::Cost_Tribute:
            for (Card *t : c.args.materials)
                if (!MoveDestroyToGY(d.field, t)) return ActionResult::Fail("cost tribute failed.");
            return ActionResult::Ok("tributes paid.");
        case ActionId::Cost_Discard: {
            int n = MoveDiscard(d.field, me, a.amount);
            return n == a.amount ? ActionResult::Ok(std::to_string(n) + " discarded.")
                                 : ActionResult::Fail("not enough cards to discard.");
        }
        case ActionId::Cost_PayLP:
            Damage(d, me, a.amount);
            return ActionResult::Ok("paid " + std::to_string(a.amount) + " LP.");
        case ActionId::Cost_BanishCost: {
            int n = 0;
            for (int i = 0; i < a.amount && !d.field.graveyardZones[me].isEmpty(); ++i)
                if (MoveBanish(d.field, d.field.graveyardZones[me].peek(-1))) ++n;
            return ActionResult::Ok(std::to_string(n) + " banished from GY (cost).");
        }

        // 2. HAND / DECK MOVEMENTS
        case ActionId::Move_Draw: {
            int n = MoveDraw(d.field, me, a.amount);
            return ActionResult::Ok("drew " + std::to_string(n) + ".");
        }
        case ActionId::Move_MillToGY: {
            int n = MoveMill(d.field, me, a.amount);
            return ActionResult::Ok("milled " + std::to_string(n) + ".");
        }
        case ActionId::Move_DiscardToGY: {
            int n = MoveDiscard(d.field, me, a.amount);
            return ActionResult::Ok("discarded " + std::to_string(n) + ".");
        }
        case ActionId::Move_ReturnHand:
            for (Card *t : resolveScope(a, c)) MoveReturnHand(d.field, t);
            return ActionResult::Ok("returned to hand.");
        case ActionId::Move_ReturnDeck:
            for (Card *t : resolveScope(a, c)) MoveReturnDeck(d.field, t);
            return ActionResult::Ok("returned to deck.");
        case ActionId::Move_SearchToHand:
            return MoveSearchToHand(d.field, me, [](const Card &) { return true; })
                       ? ActionResult::Ok("searched the deck.")
                       : ActionResult::Fail("nothing matched.");
        case ActionId::Move_Excavate: {
            auto cards = MoveExcavate(d.field, me, a.amount);
            return ActionResult::Ok("excavated " + std::to_string(cards.size()) + " card(s).");
        }

        // 3. REMOVAL & DESTRUCTION
        case ActionId::Move_DestroyToGY: {
            std::string log;
            for (Card *t : resolveScope(a, c)) {
                MoveDestroyToGY(d.field, t);
                log += t->name + " destroyed; ";
            }
            return ActionResult::Ok(log.empty() ? "nothing to destroy." : log);
        }
        case ActionId::Move_SendToGY:
            for (Card *t : resolveScope(a, c)) MoveDestroyToGY(d.field, t);
            return ActionResult::Ok("sent to the Graveyard.");
        case ActionId::Move_Banish: {
            std::string log;
            for (Card *t : resolveScope(a, c)) {
                MoveBanish(d.field, t, c.args.faceDown);
                log += t->name + " banished; ";
            }
            return ActionResult::Ok(log.empty() ? "nothing to banish." : log);
        }

        // 4. SUMMONING (special-summon placement only; composites live in Summon.hpp)
        case ActionId::Summon_Special: {
            Card *c2 = c.args.target;
            if (!c2) return ActionResult::Fail("no card to special summon.");
            return SummonToMMZ(d.field, c2, me, c.args.faceDown)
                       ? ActionResult::Ok(c2->name + " special summoned.")
                       : ActionResult::Fail("special summon failed (no free monster zone).");
        }

        // 6. CHAIN & LIFE-POINT EFFECTS
        case ActionId::LP_Damage: {
            int victim = (a.scope == Scope::Opponent) ? 1 - me : me;
            if (c.args.targetPlayer >= 0) victim = c.args.targetPlayer;
            int n = a.amount;
            if (a.scope == Scope::PerOppMonster) {
                int cnt = 0;
                for (auto &mz : d.field.monsterZones[1 - me])
                    if (mz.peek()) ++cnt;
                n *= cnt;
            }
            Damage(d, victim, n);
            return ActionResult::Ok("LP -" + std::to_string(n) + " (player " + std::to_string(victim) + ").");
        }
        case ActionId::LP_Gain: {
            int who = (a.scope == Scope::Opponent) ? 1 - me : me;
            if (c.args.targetPlayer >= 0) who = c.args.targetPlayer;
            GainLP(d, who, a.amount);
            return ActionResult::Ok("LP +" + std::to_string(a.amount) + " (player " + std::to_string(who) + ").");
        }
        case ActionId::NegateActivation:
        case ActionId::NegateEffect:
            return ActionResult::Fail("negation is resolved by the chain walker, not apply().");

        // 7. EQUIP
        case ActionId::Equip_Equip:
            if (!c.args.source || !c.args.target) return ActionResult::Fail("equip needs source and target.");
            return EquipAttach(c.args.source, c.args.target)
                       ? ActionResult::Ok(c.args.source->name + " equips " + c.args.target->name + ".")
                       : ActionResult::Fail("equip failed.");
        case ActionId::Equip_Unequip:
            return EquipDetach(c.args.target) ? ActionResult::Ok("unequipped.")
                                              : ActionResult::Fail("that card equips nothing.");

        // 8. COUNTERS
        case ActionId::Counter_Place:
            if (!c.args.target) return ActionResult::Fail("no target for the counter.");
            PlaceCounter(*c.args.target, "counter", a.amount);
            return ActionResult::Ok("counter placed.");
        case ActionId::Counter_Remove:
            if (!c.args.target) return ActionResult::Fail("no target for the counter.");
            return ActionResult::Ok("removed " + std::to_string(RemoveCounter(*c.args.target, "counter", a.amount)) + ".");

        default:
            return ActionResult::Fail("action id " + std::to_string(static_cast<int>(a.op)) +
                                      " is not realizable as an apply() op.");
    }
}

}  // namespace openjoey::engine::action
