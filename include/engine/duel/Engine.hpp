#pragma once
// ── Engine — the duel facade the app consumes (duel/Engine.hpp) ──────────────
// A thin, stateless wrapper over one external Duel: every method delegates to
// the action:: free functions. The app owns the Duel; the Engine only points
// at it (re-match = `duel_ = Duel{}` keeps the reference valid).
#include <string>
#include <vector>

#include "action/ActionArgs.hpp"
#include "engine/action/Battle.hpp"
#include "engine/action/Chains.hpp"
#include "engine/action/State.hpp"
#include "engine/action/Summons.hpp"
#include "engine/action/Turn.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Undo.hpp"

namespace openjoey::engine {

// The app says `using namespace openjoey::engine;` — pull the action
// vocabulary (findClassicEffect, classicEffectsFor, …) into that scope.
using namespace action;

class Engine {
   public:
    explicit Engine(Duel &d) : duel(d) {}

    Duel &duel;

    // ── setup ────────────────────────────────────────────────────────────────
    void setDeck(int player, const std::vector<Card *> &cards) {
        // Seal the backing: zones hold non-owning Card* into this vector, so
        // it must not be copied/resized afterwards (deckBackingMatches fails
        // if the address changes — see Duel::deckBackings).
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(),
                               [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end())
            it->second = &cards;
        else
            duel.deckBackings.push_back({player, &cards});
        action::SetDeck(duel, player, cards);
    }
    bool deckBackingMatches(int player, const void *vec) const {
        return duel.deckBackingMatches(player, vec);
    }
    // Re-point the seal at the vector that actually owns the cards (the app's
    // deck), e.g. when setDeck was fed a temporary pointer projection.
    void sealDeckBacking(int player, const void *vec) {
        auto it = std::find_if(duel.deckBackings.begin(), duel.deckBackings.end(),
                               [player](const auto &b) { return b.first == player; });
        if (it != duel.deckBackings.end())
            it->second = vec;
        else
            duel.deckBackings.push_back({player, vec});
    }
    void shuffleDecks() { action::ShuffleDecks(duel); }
    void drawOpeningHands(int n = DuelConfig::START_HAND) {
        action::DrawOpeningHands(duel, n);
    }
    void hardReset() {
        duel.field = zone::Field{};
        duel.chain.clear();
        duel.lp = {DuelConfig::START_LP, DuelConfig::START_LP};
        duel.result = DuelResult::Ongoing;
        duel.winReason = WinReason::None;
        duel.turnState = TurnState{};
        duel.battleTrace.clear();
        duel.battleStep = protocol::BattleStep::Idle;
        duel.damageStep = protocol::DamageStep::None;
        duel.lastDamageOutcome = protocol::DamageOutcome::None;
    }

    // ── turn flow ────────────────────────────────────────────────────────────
    std::string startTurn() { return action::StartTurn(duel); }
    std::string endTurn() {
        checkpoint();
        return action::EndTurn(duel);
    }
    std::string toMain1() { return action::ToMain1S(duel); }
    std::string toMain2() { return action::ToMain2S(duel); }
    std::string toBattle() { return action::ToBattleS(duel); }

    // ── battle ───────────────────────────────────────────────────────────────
    bool canAttack(Card *c) { return action::CanAttack(duel, c); }
    bool canDirectAttack(Card *c) { return action::CanDirectAttack(duel, c); }
    bool canFlipSummon(Card *c) { return action::CanFlipSummon(duel, c); }
    bool canChangePosition(Card *c) { return action::CanChangePosition(duel, c); }
    bool canActivateFromZone(const Card *c) const {
        return action::CanActivateSetSpellTrap(duel, c);
    }
    std::string declareAttack(Card *c, Card *target) {
        return action::DeclareAttack(duel, c, target);
    }
    bool confirmAttack() { return action::ConfirmAttack(duel); }
    void cancelAttack() { action::CancelAttack(duel); }
    std::string resolveDamage() {
        checkpoint();
        return action::ResolveDamage(duel);
    }

    // ── summons / positions ──────────────────────────────────────────────────
    bool canNormalSummon() { return action::CanNormalSummon(duel); }
    static int tributesRequired(const Card *c) { return action::TributesRequired(c); }
    std::string normalSummon(Card *c) {
        checkpoint();
        return action::SummonNormal(duel, c);
    }
    std::string normalSet(Card *c) {
        checkpoint();
        return action::SummonSet(duel, c);
    }
    std::string tributeSummon(Card *c, const std::vector<Card *> &tributes) {
        checkpoint();
        return action::SummonTribute(duel, c, tributes, /*faceDown=*/false);
    }
    std::string flipSummon(Card *c) {
        checkpoint();
        return action::FlipSummon(duel, c);
    }
    std::string changePosition(Card *c) {
        checkpoint();
        return action::ChangePosition(duel, c);
    }
    std::string fusionSummon(Card *f, const std::vector<Card *> &materials) {
        checkpoint();
        return action::FusionSummon(duel, f, materials);
    }
    std::string ritualSummon(Card *m, const std::vector<Card *> &tributes) {
        checkpoint();
        return action::RitualSummon(duel, m, tributes);
    }

    // ── effects / chains ─────────────────────────────────────────────────────
    std::string activateEffect(const openjoey::ActionSpec &spec, int activator,
                               const ActionArgs &args = {}) {
        checkpoint();
        return action::ActivateEffect(duel, spec, activator, args);
    }
    std::string passResponse(int player) { return action::PassResponse(duel, player); }
    bool chainWaiting() const { return action::ChainWaiting(duel); }
    std::string resolveChain() {
        checkpoint();
        return action::ResolveChain(duel);
    }

    // ── readouts ─────────────────────────────────────────────────────────────
    int lp(int player) const { return duel.lp[player]; }

    // ── Undo (bounded snapshots, one per committed action) ───────────────────
    void checkpoint() {
        undo_.push_back(makeSnapshot(duel));
        if (undo_.size() > 30)
            undo_.erase(undo_.begin());
    }
    bool canUndo() const { return !undo_.empty(); }
    bool undo() {
        if (undo_.empty())
            return false;
        restoreSnapshot(duel, undo_.back());
        undo_.pop_back();
        return true;
    }
    void clearUndo() { undo_.clear(); }

   private:
    std::vector<DuelSnapshot> undo_;
};

}  // namespace openjoey::engine