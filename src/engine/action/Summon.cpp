#include "engine/action/Summon.hpp"

namespace openjoey::engine::action {

ActionResult SummonNormal(Duel &d, Card *c) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal summon available (once per turn, Main Phase only).");
    if (TributesRequired(c) > 0) return ActionResult::Fail("level " + std::to_string(c->level) + " requires tribute(s) — use tributeSummon.");
    if (!SummonToMMZ(d.field, c, d.turnPlayer, /*faceDown=*/false)) return ActionResult::Fail("summon failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    c->state.placedThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " normal summons " + c->name + " (ATK).");
}

ActionResult SummonSet(Duel &d, Card *c) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal set available (once per turn, Main Phase only).");
    if (TributesRequired(c) > 0) return ActionResult::Fail("level " + std::to_string(c->level) + " requires tribute(s) — use tributeSummon.");
    if (!SummonToMMZ(d.field, c, d.turnPlayer, /*faceDown=*/true)) return ActionResult::Fail("set failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    c->state.setThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " sets a monster.");
}

ActionResult SummonTribute(Duel &d, Card *c, const std::vector<Card *> &tributes, bool faceDown) {
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    if (!CanNormalSummon(d)) return ActionResult::Fail("no normal summon available (once per turn, Main Phase only).");
    const int need = TributesRequired(c);
    if ((int)tributes.size() != need) return ActionResult::Fail("level " + std::to_string(c->level) + " requires exactly " + std::to_string(need) + " tribute(s).");
    for (Card *t : tributes) {
        zone::Zone *tz = d.field.monsterZoneOf(t);
        if (!tz || t->state.controller != d.turnPlayer || t == c) return ActionResult::Fail("invalid tribute: must be your own monster on the field.");
    }
    for (Card *t : tributes) MoveDestroyToGY(d.field, t);  // tributes go to the Graveyard (p.23)
    if (!SummonToMMZ(d.field, c, d.turnPlayer, faceDown)) return ActionResult::Fail("summon failed (no free monster zone).");
    d.turnState.normalSummonUsed = true;
    if (faceDown) c->state.setThisTurn = true;
    else c->state.placedThisTurn = true;
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " tribute summons " + c->name + " (" + std::to_string(need) + " tribute(s)).");
}

ActionResult FlipSummon(Duel &d, Card *c) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("Flip Summon needs a Main Phase.");
    zone::Zone *mz = d.field.monsterZoneOf(c);
    if (!mz || c->state.controller != d.turnPlayer) return ActionResult::Fail("flip summon: your set monster only.");
    if (!CanFlipSummon(d, c)) return ActionResult::Fail("cannot flip summon (face-up or set this turn).");
    if (!PosFlip(d.field, c)) return ActionResult::Fail("flip summon failed.");
    d.turnState.flipSummoned.insert(c);
    d.pendingTriggers.push_back(c);
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " flip summons " + c->name + ".");
}

ActionResult SpecialSummon(Duel &d, Card *c, SpecialPose pose) {
    if (d.result != DuelResult::Ongoing) return ActionResult::Fail("the duel is over.");
    if (!c || !c->isMonster()) return ActionResult::Fail("not a monster.");
    const bool faceDown = (pose == SpecialPose::DefDown);
    if (!SummonToMMZ(d.field, c, c->state.controller, faceDown)) return ActionResult::Fail("special summon failed (no free monster zone).");
    if (pose != SpecialPose::DefDown) c->state.placedThisTurn = true;
    else c->state.setThisTurn = true;
    return ActionResult::Ok(c->name + " special summoned.");
}

ActionResult SpecialSummon(Duel &d, Card *c, bool faceDown) {
 return SpecialSummon(d, c, faceDown ? SpecialPose::DefDown : SpecialPose::Atk); 
}

ActionResult FusionSummon(Duel &d, Card *extra, const std::vector<Card *> &materials) {
    if (d.result != DuelResult::Ongoing || (d.turn.phase != Phase::Main1 && d.turn.phase != Phase::Main2)) return ActionResult::Fail("Fusion Summon needs a Main Phase.");
    if (!extra) return ActionResult::Fail("no fusion monster.");
    if (materials.size() < 2) return ActionResult::Fail("a Fusion Summon needs at least 2 material(s).");
    auto [z, p] = d.field.findCard(extra);
    if (!z || z->type() != zone::ZoneType::ExtraDeck) return ActionResult::Fail("the fusion monster must be in its owner's Extra Deck.");
    for (Card *m : materials) {
        if (!m || m->state.controller != d.turnPlayer) return ActionResult::Fail("fusion materials must be your own monsters.");
        auto [zm, pm] = d.field.findCard(m);
        // p.22: materials come from the places the Summoning card specifies —
        // hand or field (Polymerization-style). Opponent's cards are rejected.
        if (!zm || (zm->type() != zone::ZoneType::Monster && zm->type() != zone::ZoneType::Hand)) return ActionResult::Fail("fusion materials must be monsters on the field or in your hand.");
    }
    if (!SummonToMMZ(d.field, extra, d.turnPlayer, /*faceDown=*/false)) return ActionResult::Fail("fusion summon failed (no free monster zone).");
    for (Card *m : materials) MoveDestroyToGY(d.field, m);
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " Fusion Summons " + extra->name + " (" + std::to_string(materials.size()) + " material(s) -> Graveyard).");
}

ActionResult RitualSummon(Duel &d, Card *monster, const std::vector<Card *> &tributes) {
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
    for (Card *t : tributes) MoveDestroyToGY(d.field, t);
    return ActionResult::Ok("player " + std::to_string(d.turnPlayer) + " Ritual Summons " + monster->name + " (" + std::to_string(tributes.size()) + " tribute(s) -> Graveyard).");
}

Card *SummonToken(Duel &d, const std::string &name, int atk, int def) {
 return SummonTokenMat(d.field, name, atk, def, d.turnPlayer); 
}

}  // namespace openjoey::engine::action
