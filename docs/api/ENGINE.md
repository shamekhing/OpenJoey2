# openjoey-engine — API contract

Version 0.3.0 · header-only · C++17 · raylib-free

Stable contract between `openjoey-engine` and its consumers (`openjoey-app`,
tests, the future `openjoey-ai`).

## 1. Module

* Namespace: `openjoey::engine` (zone vocabulary: `openjoey::engine::zone`).
* Target: `openjoey_engine` / alias `openjoey::engine`. Depends on
  `openjoey::cards` → `openjoey::foundation`. Raylib never enters.
* Include roots: `zone/`, `field/`, `action/`, `duel/`, `config/`, `protocol/`.

## 2. Layout & responsibilities

| Path | Owns |
|---|---|
| `zone/` | mat vocabulary: `ZoneType`, `Orientation`, `Visibility`, `IZone`, `Zone`, `ZoneStack`, `Zones` (one class per header) |
| `field/Field.hpp` | the mat: all zones both players + `tokens` (engine-spawned) + `findCard`/queries |
| `action/builtins/` | **one HPP per built-in action** (19): zone-move primitives — Draw, Mill, Discard, Destroy, Banish, ReturnHand/Deck, SearchToHand, Excavate, MaterialsToGY, ToMMZ, Normal/Set, Special, Token, Fusion, Ritual, Pos_Flip, Equip, Counters |
| `action/detail/` | ZoneMove (targeted transfer with rollback), LeaveField (detach sweeps + token erasure) |
| `action/ActionResolver.hpp` | `ActionSpec` → zone-moves/LP dispatcher (single point; `DuelContext` LP hooks) |
| `action/Catalog.hpp` | classic card → ActionSpec wiring (data table) |
| `action/actions/<Name>.hpp` ×213 | **one HPP per action** — `act_<Name>(args)` Engine members; umbrella `action/realizations.hpp` |
| `action/builtins.hpp` | umbrella for the builtins |
| `duel/Duel.hpp` | state: field, protocol, chain, LP, result/WinReason (incl. CardEffectWin) |
| `duel/Engine.hpp` | facade + per-turn bookkeeping + AI seams |
| `duel/Chain.hpp` | chain links (full ActionSpec per link) |
| `duel/Observe.hpp` | `StateView` + `Engine::observe(viewer)` — AI/UI read-only view |
| `duel/engine/*.hpp` | flow sections: Turn, Summon, Battle, Effects, Support, Observe (included inside Engine) |
| `config/DuelConfig.hpp` | the ruleset as configuration: LP/hand/deck limits, tribute brackets, seeded RNG, coin/die |
| `protocol/` | `DuelProtocol` (phase walk), `BattleProtocol` (Battle/Damage steps + outcomes), `ChainProtocol` (chain walk) |

## 3. The action system

* `openjoey::ActionId` — 213 actions (foundation); complete ruleset extraction.
* `openjoey::ActionSpec` — `{id, timing, speed, amount, lpCost, scope, needsTarget, note}`.
* `TargetScope` — named target selection (replaces sentinel protocols).
* `Engine::perform(id, args)` — **exhaustive** realization switch; the
  `perform-every-id` test fails the build if any action is unimplemented.
* Full table: [`ACTIONS.md`](ACTIONS.md).

## 4. AI seams (future `openjoey::ai`)

* `Engine::observe(viewer) → StateView` — visibility-correct read-only snapshot.
* `Engine::legalActions(player) → vector<ActionSpec>` — the action space.
* `DuelConfig` seeded RNG — deterministic episodes.

## 5. Rules notes (classic format)

* Placement truth lives in zones (`Orientation`/`Visibility`); cards carry no
  location/position copy — derive via `Field::findCard`.
* Tokens are owned by `Field::tokens` and cease to exist off the field.
* Equips grant `bonusAtk/Def` additively; detach rolls back exactly; a
  destroyed monster destroys its equips.
* Battle math uses `effectiveAtk()/effectiveDef()` (base + modifiers, ≥ 0).
* Synchro/Xyz/Pendulum/Link are explicit classic-format gates.
