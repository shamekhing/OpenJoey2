#include "engine/action/Chain.hpp"

namespace openjoey::engine::action {

ActionResult ActivateEffect(Duel &d, const ActionSpec &spec, int activator, const ActionArgs &args) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (spec.id == ActionId::None) return ActionResult::Fail("no effect to activate.");
    if ((spec.id == ActionId::NegateActivation || spec.id == ActionId::NegateEffect) && d.chain.links.empty()) return ActionResult::Fail("a negation must respond to an open chain — nothing to negate.");
    if (!d.chain.legalToChain(spec.speed)) return ActionResult::Fail("illegal chain: Spell Speed " + std::to_string(spec.speed) + " cannot join this chain.");
    // p.31: a Trap cannot be activated the same turn it was Set. The flag is
    // stamped by SeatSpellTrap and cleared by ResetPerTurnState next turn.
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

ActionResult ResolveChainImpl(Duel &d, int depth) {
    if (d.chain.links.empty()) return ActionResult::Ok("");
    if (depth > 4) return ActionResult::Fail("[chain depth cap reached — deeper links were not resolved.]");
    std::string log;
    bool unimplemented = false;  // any link action that failed to realize
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

        ActionCtx ctx{d, l.activator, l.args};
        if (l.args.spec.actions.empty()) {
            // A link with no actions list: realize the id directly when it is
            // an interpreter op (§1–8); otherwise it cannot be realized.
            Action fallback{l.id, openjoey::Scope::Target, l.args.n, false};
            ActionResult r = apply(fallback, ctx);
            log += r.msg + " ";
            if (!r.ok) unimplemented = true;
            continue;
        }
        for (const Action &a : l.args.spec.actions) {
            ActionResult r = apply(a, ctx);
            log += r.msg + " ";
            if (!r.ok) unimplemented = true;
        }
    }
    d.chain.clear();
    d.pendingTriggers.clear();  // consumed by the spec provider (none yet)
    CheckWinConditions(d);
    if (d.chain.links.empty()) d.chain.step = protocol::ChainStep::Resolved;
    return unimplemented ? ActionResult::Fail(log) : ActionResult::Ok(log);
}

ActionResult PassResponse(Duel &d, int player) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (!d.config.chainResponseWindow) return ActionResult::Fail("response window disabled — chains resolve explicitly.");
    if (d.chain.links.empty()) return ActionResult::Fail("no chain to respond to.");
    ++d.chain.consecutivePasses;
    if (d.chain.consecutivePasses < 2) return ActionResult::Ok("player " + std::to_string(player) + " passes — chain held open.");
    return ResolveChain(d);  // both passed: resolve last-activated-first
}

bool ChainWaiting(const Duel &d) {
 return d.config.chainResponseWindow && !d.chain.links.empty() && d.chain.consecutivePasses < 2; 
}

ActionResult ResolveChain(Duel &d) {
    if (d.chain.links.empty()) return ActionResult::Ok("");
    d.chain.step = protocol::ChainStep::Resolving;
    ActionResult r = ResolveChainImpl(d, 0);
    if (d.chain.links.empty()) d.chain.step = protocol::ChainStep::Resolved;
    return r;
}

}  // namespace openjoey::engine::action
