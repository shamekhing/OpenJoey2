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
explicit Engine(Duel &d);

    // ── setup (not part of the undo flow — no checkpoints) ───────────────────
void setDeck(int player, const std::vector<Card *> &cards);
bool deckBackingMatches(int player, const void *vec) const;
    // Re-point the seal at the vector that actually owns the cards (the app's
    // deck), e.g. when setDeck was fed a temporary pointer projection.
void sealDeckBacking(int player, const void *vec);
void shuffleDecks();
void drawOpeningHands(int n = DuelConfig::START_HAND);
    // Full reset by value-assignment — no hand-maintained field list, so a new
    // Duel member can never be missed. Also clears stale undo snapshots so a
    // rematch can never undo into the previous duel, and resets the recorder.
void hardReset();

    // ── replay / recording control ───────────────────────────────────────────
void setRecorder(std::unique_ptr<IRecorder> r);
void setRecording(bool on);
void seedDuel(uint32_t seed);

    // ── turn flow ────────────────────────────────────────────────────────────
ActionResult startTurn();
ActionResult endTurn();
ActionResult toMain1();
ActionResult toMain2();
ActionResult toBattle();

    // ── battle ───────────────────────────────────────────────────────────────
bool canAttack(const Card *c) const;
bool canDirectAttack(const Card *c) const;
bool canFlipSummon(const Card *c) const;
bool canChangePosition(const Card *c) const;
bool canActivateFromZone(const Card *c) const;
ActionResult declareAttack(Card *c, Card *target);
bool confirmAttack();
void cancelAttack();
ActionResult resolveDamage();

    // ── summons / positions ──────────────────────────────────────────────────
bool canNormalSummon() const;
static int tributesRequired(const Card *c);
ActionResult normalSummon(Card *c);
ActionResult normalSet(Card *c);
ActionResult tributeSummon(Card *c, const std::vector<Card *> &tributes);
ActionResult flipSummon(Card *c);
ActionResult changePosition(Card *c);
ActionResult fusionSummon(Card *f, const std::vector<Card *> &materials);
ActionResult ritualSummon(Card *m, const std::vector<Card *> &tributes);
    // Set a hand spell/trap face-down. One engine-owned implementation
    // (SeatSpellTrap via Perform) — it also stamps setThisTurn, which the
    // p.31 same-turn trap rule needs and which the old UI-side copy forgot.
ActionResult setSpellTrap(Card *c, ActionId id);

    // ── effects / chains ─────────────────────────────────────────────────────
ActionResult activateEffect(const openjoey::ActionSpec &spec, int activator, const ActionArgs &args = {});
ActionResult passResponse(int player);
bool chainWaiting() const;
ActionResult resolveChain();

    // ── readouts ─────────────────────────────────────────────────────────────
int lp(int player) const;

    // ── Undo (bounded snapshots, one per committed action) ───────────────────
void checkpoint();
bool canUndo() const;
bool undo();
void clearUndo();

   private:
    // One uniform undo policy: snapshot BEFORE the action runs. Every
    // mutator goes through this — the old facade checkpointed an arbitrary
    // subset, so phase changes and attack declarations were un-undoable.
    // Recording: after the action returns, one DuelRecord (verb + identity +
    // args + verdict + state hash) goes to the recorder; a completed EndTurn
    // additionally emits the per-turn keyframe. Replayability contract: every
    // state change flows through here — anything bypassing commit() is
    // unrecordable and therefore a bug.
void record(const char *verb, ActionId id, int activator, const ActionArgs &args, const ActionResult &r);

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