#pragma once
// ── act/summons — duel-coupled summon actions (legality + placement) ────────
#include "engine/action/Catalog.hpp"
#include "engine/action/Moves.hpp"
#include "engine/action/State.hpp"

namespace openjoey::engine::action {

using openjoey::ActionResult;

// Normal Summon (p.24): once per turn, Main Phase only, face-up ATK.
inline ActionResult SummonNormal(Duel &d, Card *c) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal summon available (once per turn, Main Phase only).");
    if (TributesRequired(c) > 0) return ActionResult::Fail("level " + std::to_string(c->level) + " requires tribute(s) — use tributeSummon.");
    if (!SummonToMMZ(d.field, c, d.turnPlayer, /*faceDown=*/false)) return ActionResult::Fail("summon failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    c->state.placedThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " normal summons " + c->name + " (ATK).");
}

// Normal Set (p.24): once per turn, Main Phase only, face-down DEF.
inline ActionResult SummonSet(Duel &d, Card *c) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal set available (once per turn, Main Phase only).");
    if (TributesRequired(c) > 0) return ActionResult::Fail("level " + std::to_string(c->level) + " requires tribute(s) — use tributeSummon.");
    if (!SummonToMMZ(d.field, c, d.turnPlayer, /*faceDown=*/true)) return ActionResult::Fail("set failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    c->state.setThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " sets a monster.");
}

// Tribute Summon/Set (p.23): send N tributes to the Graveyard, then place.
inline ActionResult SummonTribute(Duel &d, Card *c, const std::vector<Card *> &tributes, bool faceDown) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal summon available (once per turn, Main Phase only).");
    const int need = TributesRequired(c);
    if ((int)tributes.size() != need) return ActionResult::Fail("level " + std::to_string(c->level) + " requires exactly " + std::to_string(need) + " tribute(s).");
    for (Card *t : tributes) {
        zone::Zone_Monster *tz = d.field.monsterZoneOf(t);
        if (!tz || t->state.controller != d.turnPlayer || t == c) return ActionResult::Fail("invalid tribute: must be your own monster on the field.");
    }
    for (Card *t : tributes) MoveDestroyToGY(d.field, t);  // tributes go to the Graveyard (p.23)
    if (!SummonToMMZ(d.field, c, d.turnPlayer, faceDown)) return ActionResult::Fail("summon failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    if (faceDown) c->state.setThisTurn = true;
    else c->state.placedThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " tribute summons " + c->name + " (" + std::to_string(need) + " tribute(s)).");
}

// Flip Summon (p.25): your face-down Set monster -> face-up ATK; illegal the
// turn it was Set. Flip effects auto-trigger (targeted ones wait for a pick).
inline ActionResult FlipSummon(Duel &d, Card *c) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("Flip Summon needs a Main Phase.");
    zone::Zone_Monster *mz = d.field.monsterZoneOf(c);
    if (!mz || c->state.controller != d.turnPlayer) return ActionResult::Fail("not your monster in a monster zone.");
    if (mz->isVisible()) return ActionResult::Fail("monster is not face-down.");
    if (c->state.setThisTurn) return ActionResult::Fail("cannot Flip Summon the turn it was Set.");
    if (!mz->flip()) return ActionResult::Fail("flip failed.");
    d.turnState.flipSummoned.insert(c);
    std::string msg = "player " + std::to_string(d.turnPlayer) + " Flip Summons " + c->name + " (ATK).";
    bool triggered = false;
    for (const auto &e : classicEffectsFor(c->name)) {
        if (e.timing != EffectType::Trigger || e.id == ActionId::None) continue;
        if (const auto *ce = findClassicEffect(c->name); ce && ce->needsTarget) continue;  // targeted flips resolve manually
        ActionArgs fa;
        d.chain.push(e, c->state.controller, fa);
        triggered = true;
    }
    if (triggered) {
        msg += " Flip effect triggers.";
        if (!d.config.chainResponseWindow) ResolveChain(d);
    }
    return ActionResult::Ok(msg);
}

// Change battle position (p.26): face-up, once per turn, Main Phase.
inline ActionResult ChangePosition(Duel &d, Card *c) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("position change needs a Main Phase.");
    zone::Zone_Monster *mz = d.field.monsterZoneOf(c);
    if (!mz || c->state.controller != d.turnPlayer) return ActionResult::Fail("not your monster in a monster zone.");
    if (!mz->isVisible()) return ActionResult::Fail("face-down monsters are Flip Summoned, not position-changed.");
    if (c->state.placedThisTurn || c->state.setThisTurn || d.turnState.flipSummoned.count(c)) return ActionResult::Fail("cannot change position the turn it arrived.");
    if (d.turnState.attacked.count(c)) return ActionResult::Fail("cannot change position after attacking this turn (p.36).");
    if (d.turnState.positionChanged.count(c)) return ActionResult::Fail("position already changed this turn.");
    zone::Orientation to = (mz->position() == zone::Orientation::Vertical) ? zone::Orientation::Horizontal : zone::Orientation::Vertical;
    if (!mz->changeOrientation(to)) return ActionResult::Fail("position change failed.");
    d.turnState.positionChanged.insert(c);
    return ActionResult::Ok(c->name + std::string(to == zone::Orientation::Horizontal ? " switches to DEF." : " switches to ATK."));
}

// Special Summon (p.25): from ANY zone you own, in the pose of your choice —
// face-up ATK, face-up DEF, or face-down DEF.
enum class SpecialPose { Atk, DefUp, DefDown };
inline ActionResult SpecialSummon(Duel &d, Card *c, SpecialPose pose) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (!d.canAct()) return ActionResult::Fail("special summon needs an action window.");
    if (!c) return ActionResult::Fail("no card.");
    auto [z, p] = d.field.findCard(c);
    if (!z) return ActionResult::Fail("card is not in any zone.");
    if (c->state.controller != d.turnPlayer) return ActionResult::Fail("not your card.");
    if (!SummonFromZone(d.field, c, d.turnPlayer, /*toEMZ=*/false, pose == SpecialPose::Atk ? SummonPose::Atk : pose == SpecialPose::DefUp ? SummonPose::DefUp : SummonPose::DefDown)) return ActionResult::Fail("special summon failed (no free monster zone).");
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " special summons " + c->name + (pose == SpecialPose::Atk ? " (ATK)." : pose == SpecialPose::DefUp ? " (face-up DEF)." : " (face-down DEF)."));
}
// bool overload kept for existing callers (true = face-down DEF).
inline ActionResult SpecialSummon(Duel &d, Card *c, bool faceDown = false) { return SpecialSummon(d, c, faceDown ? SpecialPose::DefDown : SpecialPose::Atk); }

// Fusion Summon (p.20): Extra Deck -> EMZ; materials -> Graveyard.
inline ActionResult FusionSummon(Duel &d, Card *extra, const std::vector<Card *> &materials) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("Fusion Summon needs a Main Phase.");
    if (!extra) return ActionResult::Fail("no fusion monster.");
    if (materials.size() < 2) return ActionResult::Fail("a Fusion Summon needs at least 2 material(s).");
    auto [z, p] = d.field.findCard(extra);
    if (!z || z->type() != zone::ZoneType::ExtraDeck) return ActionResult::Fail("the fusion monster must be in its owner's Extra Deck.");
    for (Card *m : materials) {
        if (!m || m->state.controller != d.turnPlayer) return ActionResult::Fail("fusion materials must be your own monsters.");
        auto [z, p] = d.field.findCard(m);
        // p.22: materials come from the places the Summoning card specifies —
        // hand or field (Polymerization-style). Opponent's cards are rejected.
        if (!z || (z->type() != zone::ZoneType::Monster && z->type() != zone::ZoneType::Hand)) return ActionResult::Fail("fusion materials must be monsters on the field or in your hand.");
    }
    if (!PlaceFusion(d.field, extra)) return ActionResult::Fail("fusion summon failed (no free Extra Monster Zone).");
    MoveMaterialsToGY(d.field, materials);
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " Fusion Summons " + extra->name + " (" + std::to_string(materials.size()) + " material(s) -> Graveyard).");
}

// Ritual Summon (p.21): hand -> MMZ; tribute levels >= ritual level.
inline ActionResult RitualSummon(Duel &d, Card *monster, const std::vector<Card *> &tributes) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("Ritual Summon needs a Main Phase.");
    if (!monster || !monster->isMonster()) return ActionResult::Fail("not a monster.");
    auto [z, p] = d.field.findCard(monster);
    if (!z || z->type() != zone::ZoneType::Hand || monster->state.controller != d.turnPlayer) return ActionResult::Fail("the ritual monster must be in your hand.");
    if (tributes.empty()) return ActionResult::Fail("a Ritual Summon needs at least 1 tribute.");
    int total = 0;
    for (Card *t : tributes) {
        if (!t || !t->isMonster()) return ActionResult::Fail("tributes must be monsters.");
        auto [tz, tp] = d.field.findCard(t);
        if (!tz || t->state.controller != d.turnPlayer || (tz->type() != zone::ZoneType::Monster && tz->type() != zone::ZoneType::Hand)) return ActionResult::Fail("tributes must be your own monsters on the field or in hand.");
        total += t->level;
    }
    if (total < monster->level) return ActionResult::Fail("tribute levels (" + std::to_string(total) + ") are below level " + std::to_string(monster->level) + ".");
    if (!SummonToMMZ(d.field, monster, d.turnPlayer, /*faceDown=*/false)) return ActionResult::Fail("ritual summon failed (no free monster zone).");
    MoveMaterialsToGY(d.field, tributes);
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " Ritual Summons " + monster->name + " (" + std::to_string(tributes.size()) + " tribute(s) -> Graveyard).");
}

// Token (rulebook tokens): engine-spawned, owned by the mat.
inline Card *SummonToken(Duel &d, const std::string &name, int atk, int def) { return SummonTokenMat(d.field, name, atk, def, d.turnPlayer); }

}  // namespace openjoey::engine::action
