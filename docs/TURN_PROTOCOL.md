# The Game — one full state chart (start -> end)

> The entire game in one Mermaid diagram: setup, the repeating turn (Draw -> Standby ->
> Main1 -> Battle -> Main2 -> End, `turnPlayer` flipping each End), the battle walk, the
> chain overlay, and every route to a finished duel. First-turn initial conditions: turn 1
> has `skipDraw = true` and `skipBattle = true`, so its legal path is Draw(skip) ->
> Standby -> Main1 -> End.

```mermaid
stateDiagram-v2
    direction TB
    [*] --> Setup

    state "SETUP: setDeck, shuffleDecks, drawOpeningHands. turnPlayer = 0, turn 1 has skipDraw + skipBattle" as Setup

    Setup --> DrawPhase : Engine startTurn (action StartTurn)

    state "DRAW PHASE (turnPlayer draws 1)" as DrawPhase
    DrawPhase --> Standby : DrawForTurn OK then nextPhase
    DrawPhase --> Standby : skipDraw true on turn 1, then nextPhase
    DrawPhase --> DuelOver : deck out, WinReason DeckOut

    state "STANDBY PHASE (ResolveStandby scans face-up monsters for triggers)" as Standby
    Standby --> Main1 : nextPhase
    Standby --> ChainOverlay : trigger chain resolves

    state "MAIN PHASE 1 (canAct)" as Main1
    Main1 --> Main1 : SummonNormal, SummonSet (CanNormalSummon, once per turn)
    Main1 --> Main1 : TributeSummon (TributesRequired), FlipSummon (CanFlipSummon)
    Main1 --> Main1 : ChangePosition (CanChangePosition)
    Main1 --> Main1 : SetSpellTrap (stamps setThisTurn)
    Main1 --> Main1 : SpecialSummon, FusionSummon (MMZ), RitualSummon, SummonToken
    Main1 --> Main1 : EquipCard, UnequipCard, PlaceCounter, RemoveCounter
    Main1 --> ChainOverlay : ActivateEffect (ignition)
    Main1 --> BattlePhase : Engine toBattle (ToBattleS), guard not skipBattle
    Main1 --> EndPhase : Engine endTurn (EndTurn)
    Main1 --> DuelOver : lethal lpCost in ActivateEffect -> CheckWinConditions
    Main1 --> DuelOver : lethal chain resolution

    state "MAIN PHASE 2 (canAct, same action surface as Main1, no re-entering Battle)" as Main2
    Main2 --> Main2 : same actions as Main1
    Main2 --> ChainOverlay : ActivateEffect
    Main2 --> EndPhase : EndTurn
    Main2 --> DuelOver : lethal chain resolution

    state "BATTLE PHASE (unreachable on turn 1)" as BattlePhase {
        [*] --> BS_Idle
        state "BS Idle" as BS_Idle
        BS_Idle --> BS_Attacker : DeclareAttack (CanAttack, no pending, BattlePhaseOpen)
        state "BS AttackerChosen" as BS_Attacker
        BS_Attacker --> BS_Target : target opponent monster
        state "BS TargetChosen" as BS_Target
        BS_Attacker --> BS_Direct : direct attack (OpponentFieldEmpty, p.34)
        state "BS DirectDeclared" as BS_Direct
        BS_Attacker --> BS_Idle : illegal, Fail
        BS_Target --> BS_Replay : ConfirmAttack (p.37)
        state "BS ReplayCheck" as BS_Replay
        BS_Direct --> BS_Replay : ConfirmAttack
        BS_Replay --> BS_Idle : invalid, attack refunded, replayAttacker locked
        BS_Replay --> DS_Flip : valid, ResolveDamage
        state "DAMAGE Flip (face-down defender flipped, flip effects enter chain)" as DS_Flip
        DS_Flip --> DS_Compare : defender already face-up
        state "DAMAGE Compare (effectiveAtk vs ATK or DEF)" as DS_Compare
        DS_Compare --> DS_Apply : ATK vs ATK higher, destroy defender plus damage
        DS_Compare --> DS_Apply : ATK vs ATK lower, destroy attacker plus rebound
        DS_Compare --> DS_Apply : ATK vs ATK equal, both destroyed
        DS_Compare --> DS_Apply : ATK vs DEF higher, defender destroyed no damage (p.38)
        DS_Compare --> DS_Apply : ATK vs DEF lower, rebound damage
        DS_Compare --> DS_Apply : ATK vs DEF equal, nothing
        DS_Compare --> DS_Apply : direct attack, full ATK damage
        state "DAMAGE Apply (MoveDestroyToGY plus Damage, pending cleared)" as DS_Apply
        DS_Apply --> BS_Idle : resolved, CheckWinConditions, more attacks may follow
    }

    BattlePhase --> Main2 : Engine toMain2 (ToMain2S)
    BattlePhase --> EndPhase : EndTurn
    BattlePhase --> ChainOverlay : flip triggers inside the Damage Step
    BattlePhase --> DuelOver : battle damage ends the duel (LPDepletion)

    state "CHAIN OVERLAY (Spell Speed rule, reverse-order resolution)" as ChainOverlay {
        [*] --> CH_Building
        state "CH Building (links being added)" as CH_Building
        CH_Building --> CH_Building : ActivateEffect, speed >= last link, SS1 can never respond
        CH_Building --> CH_Responding : response window open (chainResponseWindow)
        state "CH Responding (PassResponse or new activation)" as CH_Responding
        CH_Responding --> CH_Building : new ActivateEffect reopens window
        CH_Responding --> CH_Responding : PassResponse first pass
        CH_Responding --> CH_Resolving : second pass or explicit ResolveChain
        state "CH Resolving (ResolveChainImpl, last link first)" as CH_Resolving
        CH_Resolving --> CH_Resolving : negated links skipped, NegateActivation blanks nearest earlier link, flip follow-ups, depth cap 4
        CH_Resolving --> CH_Done : all links done, CheckWinConditions
        state "CH Resolved" as CH_Done
        CH_Done --> [*] : chain.clear()
    }

    ChainOverlay --> Main1 : resolved, back to acting phase
    ChainOverlay --> Main2 : resolved, back to acting phase
    ChainOverlay --> DuelOver : resolution LP change ends the duel

    state "END PHASE (CanEndTurn, OverHandLimit, DiscardToHandLimit in auto mode)" as EndPhase
    EndPhase --> EndPhase : EndTurn refused, over hand limit, autoDiscard off
    EndPhase --> DrawPhase : EndTurn OK, turnPlayer flips, ++turnNumber, ResetPerTurnState

    state "DUEL OVER (Player0Win / Player1Win / Draw, LPDepletion / DeckOut / CardEffectWin)" as DuelOver
    DuelOver --> [*]
```

## Legend — chart region -> source of truth

| Chart region | Source of truth |
|---|---|
| Setup / End / DuelOver | `engine/duel/Engine.hpp`, `DuelResult`/`WinReason` (`Duel.hpp`) |
| Draw / Standby / Main / End walk | `engine/protocol/DuelProtocol.hpp` (`Phase`, `nextPhase`, `canAct`) + `engine/action/Turn.hpp` |
| Battle composite | `engine/protocol/BattleProtocol.hpp` (`BattleStep`, `DamageStep`, `DamageOutcome`) + `engine/action/Battle.hpp` |
| Chain overlay | `engine/protocol/ChainProtocol.hpp` (`ChainStep`) + `engine/duel/Chain.hpp` + `engine/action/Chains.hpp` |
| Main-phase action self-loops | `engine/action/Summon.hpp`, `Move.hpp`, `Query.hpp` guards |

## First-turn special case (the "initial conditions")

```
turn 1 (turnPlayer = 0):
    skipDraw   = true  -> Draw Phase passes with no draw
    skipBattle = true  -> Battle Phase unreachable (ToBattleS refuses)
    => legal first turn: Draw(skip) -> Standby -> Main1 -> End
```
Every later turn is the identical walk for the other `turnPlayer` — the duel is literally
"one turn protocol, repeated, with `turnPlayer` flipped and `ResetPerTurnState` in between."
