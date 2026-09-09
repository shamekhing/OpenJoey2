#pragma once
// ── Engine — the duel facade the app consumes (duel/Engine.hpp) ──────────────
// Wraps one external Duel: EVERY mutator checkpoints first (one uniform undo
// policy — phase changes and attack declarations included), then delegates to
// the action:: free functions and returns their ActionResult, so callers can
// tell success from a rules refusal without string-sniffing. The app owns the
// Duel; the Engine only points at it. The undo stack lives here (not in Duel)
// because a snapshot embeds a full Duel copy — history inside Duel would
// recurse.
#include <string>
#include <vector>

#include "action/ActionArgs.hpp"
#include "action/ActionResult.hpp"
#include "engine/action/Battle.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/Perform.hpp"
#include "engine/action/State.hpp"
#include "engine/action/Summons.hpp"
#include "engine/action/Turn.hpp"
#include "engine/duel/Duel.hpp"
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
    // rematch can never undo into the previous duel.
    void hardReset() {
        duel = Duel{};
        clearUndo();
    }

    // ── turn flow ────────────────────────────────────────────────────────────
    ActionResult startTurn() {
        return commit([&] { return action::StartTurn(duel); });
    }
    ActionResult endTurn() {
        return commit([&] { return action::EndTurn(duel); });
    }
    ActionResult toMain1() {
        return commit([&] { return action::ToMain1S(duel); });
    }
    ActionResult toMain2() {
        return commit([&] { return action::ToMain2S(duel); });
    }
    ActionResult toBattle() {
        return commit([&] { return action::ToBattleS(duel); });
    }

    // ── battle ───────────────────────────────────────────────────────────────
    bool canAttack(const Card *c) const { return action::CanAttack(duel, const_cast<Card *>(c)); }
    bool canDirectAttack(const Card *c) const { return action::CanDirectAttack(duel, const_cast<Card *>(c)); }
    bool canFlipSummon(const Card *c) const { return action::CanFlipSummon(duel, const_cast<Card *>(c)); }
    bool canChangePosition(const Card *c) const { return action::CanChangePosition(duel, const_cast<Card *>(c)); }
    bool canActivateFromZone(const Card *c) const { return action::CanActivateSetSpellTrap(duel, c); }
    ActionResult declareAttack(Card *c, Card *target) {
        return commit([&] { return action::DeclareAttack(duel, c, target); });
    }
    bool confirmAttack() { return action::ConfirmAttack(duel); }
    void cancelAttack() {
        checkpoint();
        action::CancelAttack(duel);
    }
    ActionResult resolveDamage() {
        return commit([&] { return action::ResolveDamage(duel); });
    }

    // ── summons / positions ──────────────────────────────────────────────────
    bool canNormalSummon() const { return action::CanNormalSummon(duel); }
    static int tributesRequired(const Card *c) { return action::TributesRequired(c); }
    ActionResult normalSummon(Card *c) {
        return commit([&] { return action::SummonNormal(duel, c); });
    }
    ActionResult normalSet(Card *c) {
        return commit([&] { return action::SummonSet(duel, c); });
    }
    ActionResult tributeSummon(Card *c, const std::vector<Card *> &tributes) {
        return commit([&] { return action::SummonTribute(duel, c, tributes, /*faceDown=*/false); });
    }
    ActionResult flipSummon(Card *c) {
        return commit([&] { return action::FlipSummon(duel, c); });
    }
    ActionResult changePosition(Card *c) {
        return commit([&] { return action::ChangePosition(duel, c); });
    }
    ActionResult fusionSummon(Card *f, const std::vector<Card *> &materials) {
        return commit([&] { return action::FusionSummon(duel, f, materials); });
    }
    ActionResult ritualSummon(Card *m, const std::vector<Card *> &tributes) {
        return commit([&] { return action::RitualSummon(duel, m, tributes); });
    }
    // Set a hand spell/trap face-down. One engine-owned implementation
    // (SeatSpellTrap via Perform) — it also stamps setThisTurn, which the
    // p.31 same-turn trap rule needs and which the old UI-side copy forgot.
    ActionResult setSpellTrap(Card *c, ActionId id) {
        return commit([&] {
            ActionArgs a;
            a.target = c;
            return action::Perform(duel, id, a);
        });
    }

    // ── effects / chains ─────────────────────────────────────────────────────
    ActionResult activateEffect(const openjoey::ActionSpec &spec, int activator, const ActionArgs &args = {}) {
        return commit([&] { return action::ActivateEffect(duel, spec, activator, args); });
    }
    ActionResult passResponse(int player) {
        return commit([&] { return action::PassResponse(duel, player); });
    }
    bool chainWaiting() const { return action::ChainWaiting(duel); }
    ActionResult resolveChain() {
        return commit([&] { return action::ResolveChain(duel); });
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
    template <typename F>
    ActionResult commit(F &&fn) {
        checkpoint();
        return fn();
    }

    Duel &duel;  // app-owned; intentionally NOT public any more
    std::vector<DuelSnapshot> undo_;
};

}  // namespace openjoey::engine