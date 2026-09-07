# Engine layer — API contract (`include/engine/`)

Version 0.3.0 · header-only · C++17 · raylib-free · classic ruleset only.
Namespace: `openjoey::engine` (action functions: `openjoey::engine::action`).
Consumers: `ui`, tests, the future `openjoey::ai`.

## 1. Module map

```
engine/action/   the rules, one concern per header
                 State (legality predicates, LP, win) · Moves (mat primitives)
                 Summons · Battle (declare/damage/replay) · Chains (activate/
                 resolve, response window) · Turn (start/end) · Perform (id→fn)
                 Observe (StateView + LegalActions) · Catalog (classic effects)
                 Support (standby, equips, counters, search) · Gates (format)
engine/duel/     Duel (state) · Chain · Engine (facade) · Undo (snapshots)
engine/config/   DuelConfig — constants + per-duel runtime switches
engine/field/    Field + zone/ (the mat: monster/ST/field/EMZ + stacks)
engine/protocol/ DuelProtocol, BattleProtocol, ChainProtocol (state walks)
```

## 2. `Engine` facade (`engine/duel/Engine.hpp`)

Thin wrapper over one external `Duel&` (rematch = `duel = Duel{}` +
`hardReset()`). Groups: setup (`setDeck` — seals deck backings, `shuffleDecks`,
`drawOpeningHands`), turn flow (`startTurn/endTurn/toMain1/toMain2/toBattle`),
battle (`canAttack/canDirectAttack/declareAttack/confirmAttack/cancelAttack/
resolveDamage`), summons (`normalSummon/normalSet/tributeSummon/flipSummon/
changePosition/fusionSummon/ritualSummon`), effects (`activateEffect/
passResponse/resolveChain/chainWaiting`), readouts (`lp`), AI seams
(`observe`/`legalActions` — see `include/ai/Ai.hpp`), undo
(`checkpoint/canUndo/undo/clearUndo`).

## 3. Rules as implemented (with rulebook page pins)

* **Setup**: 8000 LP, 5-card hands, turn 1 = no draw + no Battle Phase.
* **Summons** (p.23–25): Normal/Set once per turn (shared budget); tributes
  1/2 for Lv5–6/7+; Flip Summon not on the Set turn, → face-up ATK; Special
  Summons choose ATK / face-up DEF / face-down DEF (`SummonPose`); Fusion
  materials from hand or field; Ritual tribute levels ≥ level.
* **Battle** (p.34–43): ATK-vs-ATK (win/lose/tie), ATK-vs-DEF (destroy /
  nothing / rebound), direct attack only vs empty field, one attack per
  monster, face-down flip at the Damage Step, flip effects after damage,
  held-open attack with replay re-validation (p.37) — a different re-declared
  attacker locks the original.
* **Position** (p.36): not on the arrival turn, once per turn, never after
  attacking.
* **Spells/Traps** (p.31): Set Spells may activate the same turn; Set Traps
  may not (enforced twice: menu predicate `CanActivateSetSpellTrap` + the
  `args.source` check in `ActivateEffect`).
* **Chains** (p.44–47): reverse-order resolution, Spell Speed ≥ previous link,
  costs never refunded; the p.45 response window (`chainResponseWindow`)
  resolves only after both players pass.
* **End Phase** (p.41): hand limit 6 — auto-discard (`autoDiscardEndPhase`) or
  player-selected.

## 4. Effects are catalog-driven

`Card` has no effect list. `classicEffectsFor(name)` /
`findClassicEffect(name)` (`engine/action/Catalog.hpp`) return the wired
`ActionSpec`s; activations, flip triggers and standby triggers all pull from
it. Adding a card = one catalog row.

## 5. Format switches (`engine/config/DuelConfig.hpp`)

Per-duel runtime fields on `DuelConfig` (also carried in `Duel::config`):
`chainResponseWindow` (default off — chains resolve explicitly),
`autoDiscardEndPhase` (default on). Numeric rules (LP, hand limit, deck bounds,
tribute counts, skips) remain `static constexpr`.

## 6. Undo (`engine/duel/Undo.hpp`)

`makeSnapshot` / `restoreSnapshot`: zones copy as pointers (deck-owned cards
keep addresses), engine-owned **tokens deep-copy with pointer remap**
(`IZone::replacePtr` → `Field::remapPointers`), and the `CardState` of every
referenced external card is captured and re-applied — once-per-turn flags and
equip bonuses survive an undo. `Engine` keeps a bounded stack (30) fed by
`checkpoint()` in every committed action.
