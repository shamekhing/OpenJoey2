#pragma once
#include <array>
#include <set>
#include <vector>

#include "cards/Card.hpp"
#include "engine/config/DuelConfig.hpp"
#include "engine/duel/Chain.hpp"
#include "engine/field/Field.hpp"
#include "engine/protocol/BattleProtocol.hpp"
#include "engine/protocol/ChainProtocol.hpp"
#include "engine/protocol/DuelProtocol.hpp"

namespace openjoey::engine {
using cards::Card;

// ── Duel outcome (rulebook "Winning the Duel") ──────────────────────────────
enum class DuelResult : uint8_t { Ongoing,
                                  Player0Win,
                                  Player1Win,
                                  Draw };
enum class WinReason : uint8_t { None,
                                 LPDepletion,
                                 DeckOut,
                                 CardEffectWin };

// Held-open attack between Battle Step and Damage Step (replay window).
struct PendingAttack {
    Card *attacker = nullptr;
    Card *target = nullptr;  // nullptr = direct attack
    bool direct = false;
};

// ── Per-turn bookkeeping (reset at startTurn/endTurn) ────────────────────────
struct TurnState {
    bool normalSummonUsed = false;
    std::set<Card *> attacked;         // monsters that completed an attack
    std::set<Card *> flipSummoned;     // Flip Summoned this turn
    std::set<Card *> positionChanged;  // position changed this turn
    PendingAttack pending;             // attack held open
    Card *replayAttacker = nullptr;    // p.37: replay-cancelled attacker — locked
                                       // if a DIFFERENT monster re-declares
};

// ── Duel: the complete game state ────────────────────────────────────────────
// The mat, Life Points, the turn protocol, an open chain, per-turn
// bookkeeping and the battle-walk trace. No raylib; no Engine — the action
// functions (action/realize.hpp) operate directly on this.
struct Duel {
    zone::Field field;  // the mat (layer 3)
    DuelConfig config;  // the active ruleset switches (per-duel, runtime)
    DuelProtocol turn;  // phase progression
    Chain chain;        // open chain of activations

    std::array<int, 2> lp{DuelConfig::START_LP, DuelConfig::START_LP};
    int turnPlayer = 0;    // who's taking the turn
    int activePlayer = 0;  // who has priority / is acting

    DuelResult result = DuelResult::Ongoing;
    WinReason winReason = WinReason::None;

    TurnState turnState;  // once-per-turn flags + held-open attack

    // ── Deck pointer-sealing (debug guard for the non-owning Card* contract) ──
    // Zones/chain/turn-state hold raw Card* into the app's deck vectors. The
    // backing vector address is recorded at setDeck time; if the app ever
    // copies/resizes (re-allocating the buffer), backingMatches() goes false
    // and every zone pointer is dangling. Check before acting on sealed decks.
    std::vector<std::pair<int, const void *>> deckBackings;  // (player, &vector)
    bool deckBackingMatches(int player, const void *vec) const {
        for (auto &b : deckBackings)
            if (b.first == player)
                return b.second == vec;  // one recorded backing per player
        return false;                    // never sealed for this player
    }

    // Battle-walk trace (protocol walk recorded for UI/tests).
    protocol::BattleStep battleStep = protocol::BattleStep::Idle;
    protocol::DamageStep damageStep = protocol::DamageStep::None;
    protocol::DamageOutcome lastDamageOutcome = protocol::DamageOutcome::None;
    std::vector<protocol::BattleStep> battleTrace;
    void traceBattle(protocol::BattleStep s) {
        battleStep = s;
        battleTrace.push_back(s);
    }

    bool canAct() const { return turn.canAct(); }

    // Convenience: is the controller of `c` player `p`?
    bool controls(const Card *c, int p) const {
        return c && c->state.controller == p;
    }
};

}  // namespace openjoey::engine
