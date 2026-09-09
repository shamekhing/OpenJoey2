#pragma once
// ── act/chains — card-effect activation + chain resolution ──────────────────
#include "Catalog.hpp"
#include "engine/action/Moves.hpp"
#include "engine/action/State.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

// Activate a card effect: Spell Speed legality enforced; spec.lpCost charged
// here once (never refunded, even if later negated); link pushed onto chain.
inline ActionResult ActivateEffect(Duel &d, const ActionSpec &spec, int activator, const ActionArgs &args = {}) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (spec.id == ActionId::None) return ActionResult::Fail("no effect to activate.");
    if ((spec.id == ActionId::NegateActivation || spec.id == ActionId::NegateEffect) && d.chain.links.empty()) return ActionResult::Fail("a negation must respond to an open chain — nothing to negate.");
    if (!d.chain.legalToChain(spec.speed)) return ActionResult::Fail("illegal chain: Spell Speed " + std::to_string(spec.speed) + " cannot join this chain.");
    // p.31: a Trap cannot be activated the same turn it was Set. The flag is
    // stamped by SeatSpellTrap and cleared by ResetPerTurnState next turn.
    // Quick-Play Spells would need their own CardType marker before this rule
    // can distinguish them (the classic catalog has none today).
    if (args.source && args.source->isTrap() && args.source->state.setThisTurn) {
        auto [sz, sp] = d.field.findCard(args.source);
        if (sz && sz->type() == zone::ZoneType::SpellTrap && sp == activator) return ActionResult::Fail("a Trap cannot be activated the turn it was Set (p.31).");
    }
    if (spec.lpCost > 0) {
        Damage(d, activator, spec.lpCost);
        // Paying the cost can drain LP to 0 — that must be able to end the
        // duel here, not at some later win check.
        CheckWinConditions(d);
        if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the activation cost was lethal — duel over.");
    }
    d.chain.push(spec, activator, args);
    return ActionResult::Ok("player " + std::to_string(activator) + " activates — Chain Link " + std::to_string(d.chain.links.size()) + ".", spec.id);
}

// Resolve the chain: last link first. Each link's spec dispatches through the
// same mat primitives the builtins use (no second dispatcher). Negation
// blanks the responded-to link. Flipped cards' Flip effects resolve as a
// follow-up chain.
inline ActionResult ResolveChainImpl(Duel &d, int depth) {
    if (d.chain.links.empty()) return ActionResult::Ok("");
    if (depth > 4) return ActionResult::Fail("[chain depth cap reached — deeper links were not resolved.]");
    std::string log;
    bool unimplemented = false;  // any link that had no resolution handling
    std::vector<Card *> flipped;
    auto &links = d.chain.links;

    for (int i = int(links.size()) - 1; i >= 0; --i) {
        Chain::Link &l = links[i];
        if (l.negated) {
            log += "[Link " + std::to_string(i + 1) + " negated — resolves without effect.] ";
            continue;
        }
        if (l.id == ActionId::NegateActivation || l.id == ActionId::NegateEffect) {
            int j = i - 1;  // nearest previous link not already negated
            while (j >= 0 && links[j].negated) --j;
            if (j >= 0) {
                links[j].negated = true;
                log += "player " + std::to_string(l.activator) + " negates Chain Link " + std::to_string(j + 1) + ". ";
            } else {
                log += "negate fizzles (nothing to negate). ";
            }
            continue;
        }
        const ActionSpec &spec = l.spec;
        const ActionArgs &a = l.args;
        const int me = l.activator;
        const int tp = a.targetPlayer >= 0 ? a.targetPlayer : me;
        switch (l.id) {
            case ActionId::Cost_Tribute: log += MoveDestroyToGY(d.field, a.target) ? "tributed -> Graveyard. " : "tribute: no target in a zone. "; break;
            case ActionId::Cost_Discard: log += std::to_string(MoveDiscardToGY(d.field, tp, spec.amount)) + " card(s) discarded as cost. "; break;
            case ActionId::Cost_PayLP:
                Damage(d, me, spec.lpCost > 0 ? spec.lpCost : spec.amount);
                log += "player " + std::to_string(me) + " pays " + std::to_string(spec.lpCost > 0 ? spec.lpCost : spec.amount) + " LP. ";
                break;
            case ActionId::Cost_BanishCost: log += MoveBanish(d.field, a.target, /*faceDown=*/true) ? "banished (face-down) as cost. " : "banish-cost: no target in a zone. "; break;
            case ActionId::Move_Draw: log += std::to_string(MoveDraw(d.field, tp, spec.amount)) + " card(s) drawn. "; break;
            case ActionId::Move_MillToGY: log += std::to_string(MoveMillToGY(d.field, tp, spec.amount)) + " card(s) milled. "; break;
            case ActionId::Move_DiscardToGY: log += std::to_string(MoveDiscardToGY(d.field, tp, spec.amount)) + " card(s) discarded. "; break;
            case ActionId::Move_DestroyToGY:
            case ActionId::Move_SendToGY: {
                if (a.target) {
                    log += MoveDestroyToGY(d.field, a.target) ? "destroyed -> Graveyard. " : "destroy: no target. ";
                    break;
                }
                int n = MoveDestroyMass(d.field, spec.scope, me);
                log += std::to_string(n) + " card(s) destroyed -> Graveyard. ";
                break;
            }
            case ActionId::Move_Banish: log += MoveBanish(d.field, a.target, a.faceDown) ? "banished. " : "banish: no target. "; break;
            case ActionId::Move_ReturnHand: {
                if (!a.target && spec.scope == TargetScope::AllSpellsTraps) {
                    int n = 0;
                    for (int p = 0; p < 2; ++p)
                        for (auto &st : d.field.spellTrapZones[p])
                            if (Card *c = st.peek())
                                if (MoveReturnHand(d.field, c)) ++n;
                    log += std::to_string(n) + " card(s) returned to hand. ";
                    break;
                }
                Card *t = a.target;
                if (!t) {  // untargeted flip-style returns: first opp monster, else own
                    for (auto &mz : d.field.monsterZones[1 - me])
                        if (Card *c = mz.peek()) {
                            t = c;
                            break;
                        }
                    if (!t)
                        for (auto &mz : d.field.monsterZones[me])
                            if (Card *c = mz.peek()) {
                                t = c;
                                break;
                            }
                }
                log += MoveReturnHand(d.field, t) ? "returned to hand. " : "return: no target. ";
                break;
            }
            case ActionId::Move_ReturnDeck: log += MoveReturnDeck(d.field, a.target) ? "returned to deck. " : "return: no target. "; break;
            case ActionId::Summon_Flip: log += PosFlip(d.field, a.target) ? "flipped face-up (Flip effect may trigger). " : "flip: target is not a set monster. "; break;
            case ActionId::LP_Damage: {
                int n = spec.amount;
                if (spec.scope == TargetScope::PerOppMonster) {
                    int cnt = 0;
                    for (auto &mz : d.field.monsterZones[1 - me])
                        if (mz.peek()) ++cnt;
                    n *= cnt;
                }
                int victim = (spec.scope == TargetScope::Opponent) ? 1 - me : me;
                if (a.targetPlayer >= 0) victim = a.targetPlayer;
                Damage(d, victim, n);
                log += "LP -" + std::to_string(n) + " (player " + std::to_string(victim) + "). ";
                break;
            }
            case ActionId::LP_Gain: {
                int who = a.targetPlayer >= 0 ? a.targetPlayer : me;
                GainLP(d, who, spec.amount);
                log += "LP +" + std::to_string(spec.amount) + " (player " + std::to_string(who) + "). ";
                break;
            }
            default:
                // A link the resolver cannot realize must NEVER be reported as
                // a success — that is how a silent no-op hides in the log.
                log += "[NOT IMPLEMENTED — action id " + std::to_string(static_cast<int>(l.id)) + " resolved with no effect] ";
                unimplemented = true;
                break;
        }
        if (l.id == ActionId::Summon_Flip && l.args.target) flipped.push_back(l.args.target);
    }
    d.chain.clear();
    CheckWinConditions(d);
    if (!flipped.empty()) {
        bool pushed = false;
        for (Card *c : flipped)
            for (const auto &e : classicEffectsFor(c->name))
                if (e.timing == EffectType::Trigger && e.id != ActionId::None) {
                    ActionArgs fa;
                    d.chain.push(e, c->state.controller, fa);
                    pushed = true;
                }
        if (pushed) {
            log += "Flip effects trigger. ";
            ActionResult follow = ResolveChainImpl(d, depth + 1);
            log += follow.msg;
            if (!follow.ok) unimplemented = true;
        }
    }
    if (d.chain.links.empty()) d.chain.step = protocol::ChainStep::Resolved;
    return unimplemented ? ActionResult::Fail(log) : ActionResult::Ok(log);
}

// Pass the response window (p.45). With d.config.chainResponseWindow
// enabled, a chain resolves only once BOTH players pass consecutively; any
// new activation (ActivateEffect) reopens the window. With the flag off
// (default), chains are resolved explicitly and passing is a no-op.
inline ActionResult ResolveChain(Duel &d);  // fwd: PassResponse resolves on both-pass

inline ActionResult PassResponse(Duel &d, int player) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (!d.config.chainResponseWindow) return ActionResult::Fail("response window disabled — chains resolve explicitly.");
    if (d.chain.links.empty()) return ActionResult::Fail("no chain to respond to.");
    ++d.chain.consecutivePasses;
    if (d.chain.consecutivePasses < 2) return ActionResult::Ok("player " + std::to_string(player) + " passes — chain held open.");
    return ResolveChain(d);  // both passed: resolve last-activated-first
}

inline bool ChainWaiting(const Duel &d) { return d.config.chainResponseWindow && !d.chain.links.empty() && d.chain.consecutivePasses < 2; }

inline ActionResult ResolveChain(Duel &d) {
    if (d.chain.links.empty()) return ActionResult::Ok("");
    d.chain.step = protocol::ChainStep::Resolving;
    ActionResult r = ResolveChainImpl(d, 0);
    if (d.chain.links.empty()) d.chain.step = protocol::ChainStep::Resolved;
    return r;
}

}  // namespace openjoey::engine::action
