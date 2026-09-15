# Duel State Layer — reference

> Canonical map of where state lives, why it lives there, and what is still missing.
> Read bottom-up: `Duel` owns everything; the `action` layer mutates it; `Engine` checkpoints/undo.

## 1. Layer map (compile order)

```
cards            ->  Card, CardState, enums          (identity + per-card flags)
engine/field     ->  Zone* / ZoneStack / ZoneSpread / Field   (the mat; raw Card* holders)
engine/duel      ->  Duel, Chain, DuelProtocol       (state container + protocol enums)
engine/protocol  ->  Phase, BattleStep, DamageStep, DamageOutcome, ChainStep
engine/action    ->  ActionId/Spec/Args vocabulary, Move/Query/Summon/Battle/Chain/Turn
ui               ->  raylib rendering of the protocols + field
```

- **field** = "where is a card". Owns no cards, holds raw `Card*`.
- **duel** = "what the game state is right now" (`Duel`) + "what walk we're on" (`DuelProtocol`, `Chain`, battle/damage protocols).
- **action** = "what a legal move realizes into" (pure functions `ActionResult(...)` over `Duel&`).
- **Engine** = app-facing; **every mutator checkpoints first** (bounded 30-deep undo).

## 2. `Duel` — the complete game state (`include/engine/duel/Duel.hpp`)

| member | type | role |
|---|---|---|
| `field` | `zone::Field` | the mat (layer 3) |
| `config` | `DuelConfig` | runtime ruleset switches |
| `turn` | `DuelProtocol` | phase + turn number + first-turn restrictions |
| `chain` | `Chain` | open chain of activations |
| `lp` | `array<int,2>` | Life Points [P0,P1] |
| `turnPlayer` / `activePlayer` | `int` | whose turn / who has priority |
| `result` / `winReason` | `DuelResult` / `WinReason` | terminal verdict |
| `turnState` | `TurnState` | once-per-turn flags + held-open attack |
| `rng` | `std::mt19937` | the duel-owned shuffle engine (replay determinism) |
| `deckBackings` | `vector<pair<int,const void*>>` | debug seal: card* point into the app's deck vectors |
| `battleStep` / `damageStep` / `lastDamageOutcome` / `battleTrace` | protocol enums | Battle Phase walk + trace for UI/tests |
| `pendingTriggers` | `vector<Card*>` | hook: flipped/standby-ready cards awaiting a spec provider (no hardcoding) |

### `PendingAttack` — handle, not snapshot
Held-open attack between Battle Step and Damage Step (replay window):
```cpp
Card *attacker = nullptr;
Card *target  = nullptr;   // nullptr == direct attack
bool direct = false;
```
Intentionally carries NO stats snapshot — `ResolveDamage` recomputes `effectiveAtk()` and `ConfirmAttack` re-litigates; correct for classic (replay = re-legalize-then-attack).

### `TurnState`
```cpp
bool  normalSummonUsed = false;      // Normal Summon/Set share the once-per-turn quota (correct)
bool  drawDone = false;              // Draw Phase guard: one draw per turn, Draw Phase only
set<Card*> attacked;                 // monsters that completed an attack this turn
set<Card*> flipSummoned;             // Flip Summoned this turn
set<Card*> positionChanged;          // position changed this turn
PendingAttack pending;               // attack held open
Card* replayAttacker = nullptr;      // p.37: replay-locked attacker
```
Gaps to watch: per-turn flags are split between `CardState` (`setThisTurn`/`placedThisTurn`) and `TurnState` (the sets above) — both cleared wholesale by `ResetPerTurnState`; works, but one concept in two homes.

## 3. Protocols

`DuelProtocol` (turn): `Draw -> Standby -> Main1 -> Battle -> Main2 -> End` (End wraps + `++turnNumber`). `nextPhase()` ignores `result`. `canAct()` = Main1|Main2|Battle only. `skipDraw`/`skipBattle` gate turn 1. Drivers live in `action/Turn.hpp`: `StartTurn / EndTurn / ToMain1S / ToBattleS / ToMain2S / ResetPerTurnState / DrawForTurn / SetDeck / ShuffleDecks / DrawOpeningHands / ResolveStandby`.

`BattleProtocol` (battle): `BattleStep` `Idle -> AttackerChosen -> (TargetChosen|DirectDeclared) -> ReplayCheck -> DamageBegin -> FlipRevealed -> DamageCalculated -> (Resolved|Cancelled)`; `DamageStep` `None -> Flip -> Calculate -> Compare -> Apply -> End`; 8 `DamageOutcome` values. `d.traceBattle(s)` appends to `battleTrace`.

`Chain` + `ChainProtocol`: `Idle|Building|Responding|Resolving|Resolved`. `push()` sets `Building` and resets `consecutivePasses` (opens the response window). `legalToChain(speed)` = empty-chain-true, else `speed>1 && speed>=back().speed` (no SS1 responses). `resolutionOrder()` reverses (last first). `clear()` resets all.

### Chain resolution walk (`Chains.hpp`)
- `ActivateEffect` guards: `result==Ongoing`, `id!=None`, negation needs an open chain, `legalToChain(speed)`, p.31 trap-same-turn-set, `lpCost` charged + `CheckWinConditions` immediately (a lethal cost ends the duel here, not at a later check).
- `ResolveChainImpl(d, depth)`: reverse order; negated links skipped; `NegateActivation/NegateEffect` blanks the nearest earlier non-negated link; per-link realization through the `Action` interpreter; **never a silent success**; flip follow-ups; depth cap 4.
- `PassResponse`: `++consecutivePasses`; at 2 => `ResolveChain`; no-op when `chainResponseWindow` off.
- `ResolveChain`: `step=Resolving` -> impl -> `Resolved` if empty.

## 4. `Engine` facade (`include/engine/duel/Engine.hpp`)
App owns `Duel`; Engine holds `Duel&` (no mat copy). **One undo policy:** every mutator is `commit(...)` => `checkpoint()` a deep `DuelSnapshot` BEFORE running; snapshots bounded to 30; `undo()` restores. `setDeck`/`sealDeckBacking`/`deckBackingMatches` enforce the non-owning-Card* seal (deck vector must not reallocate). Recording: every commit appends a `DuelRecord` (verb/id/activator/args/verdict/stateHash) to the optional recorder; a completed `endTurn` emits the per-turn keyframe.

## 5. How the action layer is meant to be used
1. **Player/UI:** call `Engine::xxx` (e.g. `normalSummon(c)`). Facade checkpoints, calls the action function, returns `ActionResult` — ok/msg = success vs exact refusal. Never string-sniff the mat.
2. **Spec-driven path:** an `ActionSpec` (id + `Action`s + scope + amount + speed) flows through `ActivateEffect` onto the chain and is realized by the resolver through the same mat primitives — no second code path.
3. **AI seam:** `Observe(duel, viewer)` => `StateView`; `LegalActions` => verb list (opponent hand hidden).
4. **Tests:** assert on `battleTrace`/`DuelResult`, not UI strings.

## 6. Quick reference: protocol step walks
```
TURN:  Draw -> [standby triggers] -> Standby -> Main1 -> Battle -> Main2 -> End -> (next turn)
BATTLE: Idle -> AttackerChosen -> (TargetChosen|DirectDeclared) -> ReplayCheck
        -> DamageBegin -> [FlipRevealed] -> DamageCalculated -> Resolved|Cancelled
DAMAGE: None -> Flip -> Calculate -> Compare -> Apply -> End
CHAIN:  Idle -> Building -> Responding -> Resolving(last->first) -> Resolved
        response window: both players pass (consecutivePasses==2) => resolve
```

