// Engine implementation (duel/Engine.cpp).
#include "engine/duel/Engine.hpp"

#include <algorithm>
#include <utility>

namespace openjoey::engine {

Engine::Engine(Duel &d) : duel(d) {
}

void Engine::setDeck(int player, const std::vector<Card *> &cards) {
        // Seal the backing: zones hold non-owning Card* into this vector, so
        // it must not be copied/resized afterwards (deckBackingMatches fails
        // if the address changes — see Duel::deckBackings).
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(), [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end()) it->second = &cards;
        else duel.deckBackings.push_back({player, &cards});
        action::SetDeck(duel, player, cards);
    
}

bool Engine::deckBackingMatches(int player, const void *vec) const {
 return duel.deckBackingMatches(player, vec); 
}

void Engine::sealDeckBacking(int player, const void *vec) {
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(), [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end()) it->second = vec;
        else duel.deckBackings.push_back({player, vec});
    
}

void Engine::shuffleDecks() {
 action::ShuffleDecks(duel); 
}

void Engine::drawOpeningHands(int n) {
 action::DrawOpeningHands(duel, n); 
}

void Engine::hardReset() {
        duel = Duel{};
        clearUndo();
        if (recorder) recorder->onReset();
    
}

void Engine::setRecorder(std::unique_ptr<IRecorder> r) {
 recorder = std::move(r); 
}

void Engine::setRecording(bool on) {
 recording = on; 
}

void Engine::seedDuel(uint32_t seed) {
 duel.seedRng(seed); 
}

ActionResult Engine::startTurn() {
        return commit("startTurn", [&] { return action::StartTurn(duel); });
    
}

ActionResult Engine::endTurn() {
        ActionArgs a;
        return commit("endTurn", ActionId::EndTurn, duel.turnPlayer, a, [&] { return action::EndTurn(duel); });
    
}

ActionResult Engine::toMain1() {
        return commit("toMain1", [&] { return action::ToMain1S(duel); });
    
}

ActionResult Engine::toMain2() {
        return commit("toMain2", [&] { return action::ToMain2S(duel); });
    
}

ActionResult Engine::toBattle() {
        return commit("toBattle", [&] { return action::ToBattleS(duel); });
    
}

bool Engine::canAttack(const Card *c) const {
 return action::CanAttack(duel, const_cast<Card *>(c)); 
}

bool Engine::canDirectAttack(const Card *c) const {
 return action::CanDirectAttack(duel, const_cast<Card *>(c)); 
}

bool Engine::canFlipSummon(const Card *c) const {
 return action::CanFlipSummon(duel, const_cast<Card *>(c)); 
}

bool Engine::canChangePosition(const Card *c) const {
 return action::CanChangePosition(duel, const_cast<Card *>(c)); 
}

bool Engine::canActivateFromZone(const Card *c) const {
 return action::CanActivateSetSpellTrap(duel, c); 
}

ActionResult Engine::declareAttack(Card *c, Card *target) {
        ActionArgs a;
        a.source = c;
        a.target = target;
        return commit("declareAttack", ActionId::DeclareAttack, duel.turnPlayer, a, [&] { return action::DeclareAttack(duel, c, target); });
    
}

bool Engine::confirmAttack() {
 return action::ConfirmAttack(duel); 
}

void Engine::cancelAttack() {
        Card *attacker = duel.turnState.pending.attacker;
        Card *target = duel.turnState.pending.target;
        checkpoint();
        action::CancelAttack(duel);
        ActionArgs a;
        a.source = attacker;
        a.target = target;
        record("cancelAttack", ActionId::CancelAttack, duel.turnPlayer, a, ActionResult::Ok("attack cancelled."));
    
}

ActionResult Engine::resolveDamage() {
        return commit("resolveDamage", [&] { return action::ResolveDamage(duel); });
    
}

bool Engine::canNormalSummon() const {
 return action::CanNormalSummon(duel); 
}

int Engine::tributesRequired(const Card *c) {
 return action::TributesRequired(c); 
}

ActionResult Engine::normalSummon(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("normalSummon", ActionId::Summon_Normal, duel.turnPlayer, a, [&] { return action::SummonNormal(duel, c); });
    
}

ActionResult Engine::normalSet(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("normalSet", ActionId::Summon_Set, duel.turnPlayer, a, [&] { return action::SummonSet(duel, c); });
    
}

ActionResult Engine::tributeSummon(Card *c, const std::vector<Card *> &tributes) {
        ActionArgs a;
        a.target = c;
        a.materials = tributes;
        return commit("tributeSummon", ActionId::TributeSummon, duel.turnPlayer, a, [&] { return action::SummonTribute(duel, c, tributes, /*faceDown=*/false); });
    
}

ActionResult Engine::flipSummon(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("flipSummon", ActionId::FlipSummon, duel.turnPlayer, a, [&] { return action::FlipSummon(duel, c); });
    
}

ActionResult Engine::changePosition(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("changePosition", ActionId::ChangeMonsterBattlePosition, duel.turnPlayer, a, [&] {
            auto *mz = duel.field.monsterZoneOf(c);
            if (!mz) return ActionResult::Fail("position change: your monster on the field only.");
            zone::Orientation to = (mz->orientation() == zone::Orientation::Vertical) ? zone::Orientation::Horizontal
                                                                                     : zone::Orientation::Vertical;
            return action::PosChange(duel.field, c, to) ? ActionResult::Ok(c->name + " position changed.")
                                                        : ActionResult::Fail("position change failed.");
        });
    
}

ActionResult Engine::fusionSummon(Card *f, const std::vector<Card *> &materials) {
        ActionArgs a;
        a.target = f;
        a.materials = materials;
        return commit("fusionSummon", ActionId::Summon_Fusion, duel.turnPlayer, a, [&] { return action::FusionSummon(duel, f, materials); });
    
}

ActionResult Engine::ritualSummon(Card *m, const std::vector<Card *> &tributes) {
        ActionArgs a;
        a.target = m;
        a.materials = tributes;
        return commit("ritualSummon", ActionId::Summon_Ritual, duel.turnPlayer, a, [&] { return action::RitualSummon(duel, m, tributes); });
    
}

ActionResult Engine::setSpellTrap(Card *c, ActionId id) {
        ActionArgs a;
        a.target = c;
        return commit("setSpellTrap", id, duel.turnPlayer, a, [&] {
            return action::SeatSpellTrap(duel.field, a.target)
                       ? ActionResult::Ok(a.target->name + " set.")
                       : ActionResult::Fail("set failed (no free spell/trap zone).");
        });
    
}

ActionResult Engine::passResponse(int player) {
        ActionArgs a;
        a.targetPlayer = player;
        return commit("passResponse", ActionId::PassChain, player, a, [&] { return action::PassResponse(duel, player); });
    
}

bool Engine::chainWaiting() const {
 return action::ChainWaiting(duel); 
}

ActionResult Engine::resolveChain() {
        return commit("resolveChain", [&] { return action::ResolveChain(duel); });
    
}

int Engine::lp(int player) const {
 return duel.lp[player]; 
}

void Engine::checkpoint() {
        undo_.push_back(makeSnapshot(duel));
        if (undo_.size() > 30) undo_.erase(undo_.begin());
    
}

bool Engine::canUndo() const {
 return !undo_.empty(); 
}

bool Engine::undo() {
        if (undo_.empty()) return false;
        restoreSnapshot(duel, undo_.back());
        undo_.pop_back();
        return true;
    
}

void Engine::clearUndo() {
 undo_.clear(); 
}

void Engine::record(const char *verb, ActionId id, int activator, const ActionArgs &args, const ActionResult &r) {
        if (!recorder || !recording) return;
        DuelRecord rec;
        rec.verb = verb ? verb : "";
        rec.turnNumber = duel.turn.turnNumber;
        rec.turnPlayer = duel.turnPlayer;
        rec.activator = activator;
        rec.id = id;
        rec.args = args;
        rec.result = r;
        rec.hash = duel.stateHash();
        recorder->onAction(rec);
        if (verb && r.ok && std::string_view(verb) == "endTurn")
            recorder->onKeyframe(DuelKeyframe{makeSnapshot(duel)});
    
}

ActionResult Engine::activateEffect(const openjoey::ActionSpec &spec, int activator, const ActionArgs &args) {
    ActionArgs a = args;
    a.spec = spec;
    return commit("activateEffect", spec.id, activator, a, [&] { return action::ActivateEffect(duel, spec, activator, args); });
}

}  // namespace openjoey::engine
