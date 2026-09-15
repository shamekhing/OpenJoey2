#pragma once
// ── Engine — the duel facade the app consumes (duel/Engine.hpp) ──────────────
// Wraps one external Duel: EVERY mutator checkpoints first (one uniform undo
// policy — phase changes and attack declarations included), then delegates to
// the action:: free functions and returns their ActionResult, so callers can
// tell success from a rules refusal without string-sniffing. The app owns the
// Duel; the Engine only points at it. The undo stack lives here (not in Duel)
// because a snapshot embeds a full Duel copy — history inside Duel would
// recurse.
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "action/ActionArgs.hpp"
#include "action/ActionResult.hpp"
#include "engine/action/Battle.hpp"
#include "engine/action/Chain.hpp"
#include "engine/action/Query.hpp"
#include "engine/action/Summon.hpp"
#include "engine/action/Turn.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Recorder.hpp"
#include "engine/duel/Undo.hpp"

namespace openjoey::engine {

// The app says `using namespace openjoey::engine;` — pull the action
// vocabulary (findClassicEffect, classicEffectsFor, …) into that scope.
using namespace action;
using openjoey::ActionResult;

class Engine {
   public:
    explicit Engine(Duel &d) : duel(d) {}

    // ── setup (not part of the undo flow — no checkpoints) ───────────────────
    void setDeck(int player, const std::vector<Card *> &cards) {
        // Seal the backing: zones hold non-owning Card* into this vector, so
        // it must not be copied/resized afterwards (deckBackingMatches fails
        // if the address changes — see Duel::deckBackings).
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(), [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end()) it->second = &cards;
        else duel.deckBackings.push_back({player, &cards});
        action::SetDeck(duel, player, cards);
    }
    bool deckBackingMatches(int player, const void *vec) const { return duel.deckBackingMatches(player, vec); }
    // Re-point the seal at the vector that actually owns the cards (the app's
    // deck), e.g. when setDeck was fed a temporary pointer projection.
    void sealDeckBacking(int player, const void *vec) {
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(), [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end()) it->second = vec;
        else duel.deckBackings.push_back({player, vec});
    }
    void shuffleDecks() { action::ShuffleDecks(duel); }
    void drawOpeningHands(int n = DuelConfig::START_HAND) { action::DrawOpeningHands(duel, n); }
    // Full reset by value-assignment — no hand-maintained field list, so a new
    // Duel member can never be missed. Also clears stale undo snapshots so a
    // rematch can never undo into the previous duel, and resets the recorder.
    void hardReset() {
        duel = Duel{};
        clearUndo();
        if (recorder) recorder->onReset();
    }

    // ── replay / recording control ───────────────────────────────────────────
    void setRecorder(std::unique_ptr<IRecorder> r) { recorder = std::move(r); }
    void setRecording(bool on) { recording = on; }
    void seedDuel(uint32_t seed) { duel.seedRng(seed); }  // header-time, like setDeck

    // ── turn flow ────────────────────────────────────────────────────────────
    ActionResult startTurn() {
        return commit("startTurn", [&] { return action::StartTurn(duel); });
    }
    ActionResult endTurn() {
        ActionArgs a;
        return commit("endTurn", ActionId::EndTurn, duel.turnPlayer, a, [&] { return action::EndTurn(duel); });
    }
    ActionResult toMain1() {
        return commit("toMain1", [&] { return action::ToMain1S(duel); });
    }
    ActionResult toMain2() {
        return commit("toMain2", [&] { return action::ToMain2S(duel); });
    }
    ActionResult toBattle() {
        return commit("toBattle", [&] { return action::ToBattleS(duel); });
    }

    // ── battle ───────────────────────────────────────────────────────────────
    bool canAttack(const Card *c) const { return action::CanAttack(duel, const_cast<Card *>(c)); }
    bool canDirectAttack(const Card *c) const { return action::CanDirectAttack(duel, const_cast<Card *>(c)); }
    bool canFlipSummon(const Card *c) const { return action::CanFlipSummon(duel, const_cast<Card *>(c)); }
    bool canChangePosition(const Card *c) const { return action::CanChangePosition(duel, const_cast<Card *>(c)); }
    bool canActivateFromZone(const Card *c) const { return action::CanActivateSetSpellTrap(duel, c); }
    ActionResult declareAttack(Card *c, Card *target) {
        ActionArgs a;
        a.source = c;
        a.target = target;
        return commit("declareAttack", ActionId::DeclareAttack, duel.turnPlayer, a, [&] { return action::DeclareAttack(duel, c, target); });
    }
    bool confirmAttack() { return action::ConfirmAttack(duel); }
    void cancelAttack() {
        Card *attacker = duel.turnState.pending.attacker;
        Card *target = duel.turnState.pending.target;
        checkpoint();
        action::CancelAttack(duel);
        ActionArgs a;
        a.source = attacker;
        a.target = target;
        record("cancelAttack", ActionId::CancelAttack, duel.turnPlayer, a, ActionResult::Ok("attack cancelled."));
    }
    ActionResult resolveDamage() {
        return commit("resolveDamage", [&] { return action::ResolveDamage(duel); });
    }

    // ── summons / positions ──────────────────────────────────────────────────
    bool canNormalSummon() const { return action::CanNormalSummon(duel); }
    static int tributesRequired(const Card *c) { return action::TributesRequired(c); }
    ActionResult normalSummon(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("normalSummon", ActionId::Summon_Normal, duel.turnPlayer, a, [&] { return action::SummonNormal(duel, c); });
    }
    ActionResult normalSet(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("normalSet", ActionId::Summon_Set, duel.turnPlayer, a, [&] { return action::SummonSet(duel, c); });
    }
    ActionResult tributeSummon(Card *c, const std::vector<Card *> &tributes) {
        ActionArgs a;
        a.target = c;
        a.materials = tributes;
        return commit("tributeSummon", ActionId::TributeSummon, duel.turnPlayer, a, [&] { return action::SummonTribute(duel, c, tributes, /*faceDown=*/false); });
    }
    ActionResult flipSummon(Card *c) {
        ActionArgs a;
        a.target = c;
        return commit("flipSummon", ActionId::FlipSummon, duel.turnPlayer, a, [&] { return action::FlipSummon(duel, c); });
    }
    ActionResult changePosition(Card *c) {
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
    ActionResult fusionSummon(Card *f, const std::vector<Card *> &materials) {
        ActionArgs a;
        a.target = f;
        a.materials = materials;
        return commit("fusionSummon", ActionId::Summon_Fusion, duel.turnPlayer, a, [&] { return action::FusionSummon(duel, f, materials); });
    }
    ActionResult ritualSummon(Card *m, const std::vector<Card *> &tributes) {
        ActionArgs a;
        a.target = m;
        a.materials = tributes;
        return commit("ritualSummon", ActionId::Summon_Ritual, duel.turnPlayer, a, [&] { return action::RitualSummon(duel, m, tributes); });
    }
    // Set a hand spell/trap face-down. One engine-owned implementation
    // (SeatSpellTrap via Perform) — it also stamps setThisTurn, which the
    // p.31 same-turn trap rule needs and which the old UI-side copy forgot.
    ActionResult setSpellTrap(Card *c, ActionId id) {
        ActionArgs a;
        a.target = c;
        return commit("setSpellTrap", id, duel.turnPlayer, a, [&] {
            return action::SeatSpellTrap(duel.field, a.target)
                       ? ActionResult::Ok(a.target->name + " set.")
                       : ActionResult::Fail("set failed (no free spell/trap zone).");
        });
    }

    // ── effects / chains ─────────────────────────────────────────────────────
    ActionResult activateEffect(const openjoey::ActionSpec &spec, int activator, const ActionArgs &args = {}) {
        ActionArgs a = args;
        a.spec = spec;
        return commit("activateEffect", spec.id, activator, a, [&] { return action::ActivateEffect(duel, spec, activator, args); });
    }
    ActionResult passResponse(int player) {
        ActionArgs a;
        a.targetPlayer = player;
        return commit("passResponse", ActionId::PassChain, player, a, [&] { return action::PassResponse(duel, player); });
    }
    bool chainWaiting() const { return action::ChainWaiting(duel); }
    ActionResult resolveChain() {
        return commit("resolveChain", [&] { return action::ResolveChain(duel); });
    }

    // ── readouts ─────────────────────────────────────────────────────────────
    int lp(int player) const { return duel.lp[player]; }

    // ── Undo (bounded snapshots, one per committed action) ───────────────────
    void checkpoint() {
        undo_.push_back(makeSnapshot(duel));
        if (undo_.size() > 30) undo_.erase(undo_.begin());
    }
    bool canUndo() const { return !undo_.empty(); }
    bool undo() {
        if (undo_.empty()) return false;
        restoreSnapshot(duel, undo_.back());
        undo_.pop_back();
        return true;
    }
    void clearUndo() { undo_.clear(); }

   private:
    // One uniform undo policy: snapshot BEFORE the action runs. Every
    // mutator goes through this — the old facade checkpointed an arbitrary
    // subset, so phase changes and attack declarations were un-undoable.
    // Recording: after the action returns, one DuelRecord (verb + identity +
    // args + verdict + state hash) goes to the recorder; a completed EndTurn
    // additionally emits the per-turn keyframe. Replayability contract: every
    // state change flows through here — anything bypassing commit() is
    // unrecordable and therefore a bug.
    void record(const char *verb, ActionId id, int activator, const ActionArgs &args, const ActionResult &r) {
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

    template <typename F>
    ActionResult commit(const char *verb, ActionId id, int activator, const ActionArgs &args, F &&fn) {
        checkpoint();
        ActionResult r = fn();
        record(verb, id, activator, args, r);
        return r;
    }

    // Convenience for mutators without action identity (phase moves, battle steps).
    template <typename F>
    ActionResult commit(const char *verb, F &&fn) {
        return commit(verb, ActionId::None, duel.turnPlayer, ActionArgs{}, std::forward<F>(fn));
    }

    Duel &duel;  // app-owned; intentionally NOT public any more
    std::vector<DuelSnapshot> undo_;
    std::unique_ptr<IRecorder> recorder;  // passive observer; never mutates the duel
    bool recording = true;
};

}  // namespace openjoey::engine