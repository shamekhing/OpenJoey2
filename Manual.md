# OpenJoey2 — Runtime Execution Manual

Not an API reference. This is the **execution order of the program**, from
process start (`main`) to process exit, traced function-by-function through
the **four-repo architecture** (`openjoey-foundation → openjoey-cards →
openjoey-engine → openjoey-app`). For every stage it states **which values
change** — whose state moves, where pointers land, which enums flip.

Everything below is read from the actual headers/sources. File paths are
relative to each repo root. Namespaces: `openjoey` (foundation),
`openjoey::cards`, `openjoey::engine`, `openjoey::ui` + `openjoey::app`.

---

## 0. Map of the runtime layers (who runs inside whom)

```
main()                                    openjoey-app/src/main.cpp
└─ openjoey::app::run(argc, argv)
   └─ openjoey::ui::App app(argv0)        include/ui/core/App.hpp
      ├─ Config::Load(argv0)              foundation: layered settings
      ├─ PlatformContext (raylib window)  ui/platform/
      ├─ CardDatabase cardDb_             openjoey-cards
      ├─ CardImageCache imageCache_       ui/cards/ (async art loading)
      ├─ AppContext ctx_                  references to all of the above
      └─ ScreenManager screenManager_     the screen stack
         └─ Run() loop:                   Update → handleEvent → Draw
            └─ IScreen implementations    ui/screens/*.hpp
               └─ DuelScreen              owns the entire duel stack:
                  ├─ openjoey::engine::Duel + Engine     (openjoey-engine)
                  ├─ DuelSetup / DuelActions / DuelEffects / FieldGrid
                  └─ DuelUIState (mode machine)
```

Dependency direction: `openjoey-app → openjoey-engine → openjoey-cards →
openjoey-foundation`. Raylib exists only in openjoey-app. The future
`openjoey::ai` hangs off the engine with the same headless guarantees.

---

## 1. Process start

```cpp
int main(int argc, char** argv) { return openjoey::app::run(argc, argv); }
```

`openjoey::app::run` (src/main.cpp) constructs `openjoey::ui::App(argv0)`.
The App constructor member-initializes, **in declaration order** (order
matters: everything `ctx_` references must exist first):

1. `settings_ = Config::Load(argv0)` — **openjoey-foundation**
   `Config::Load`: compiled defaults → `data/settings.json` (shipped
   reference, resolved beside the exe, else CWD) → `data/user_settings.json`
   (app-written overlay, wins). Values that change: `paths.cardsJson`,
   `paths.cardImgDir`, `paths.cardImgUrl/-SmallUrl` (remote endpoints come
   only from config, never compiled), `screenWidth/Height`, `targetFps`,
   `fullscreen`, `downloadImages`. `baseDir_` lands on the resolved data dir.
2. `platform_(settings_)` — **PlatformContext**: opens the raylib window
   (`InitWindow(screenWidth, screenHeight, windowTitle)`), `SetTargetFPS`.
   From here on, every frame costs real GPU time.
3. `cardDb_` — empty `openjoey::cards::CardDatabase` (pointer-stable storage).
4. `selectedDeck_` — empty `vector<Card>`; the deck editor fills it.
5. `imageCache_(cardImgDir, cardImgUrl, cardImgSmallUrl)` — async art loader.
6. `ctx_{cardDb_, selectedDeck_, imageCache_, settings_}` — `AppContext`
   holds **references**: every screen sees the same DB, deck and settings.
7. `screenManager_` — empty stack.

## 2. Boot into the first screen

`App::Run()` (App.hpp):

1. `LoadCards()` — `cardDb_.LoadFromFile(settings_.paths.cardsJson)`:
   `cards::parseRemoteCardJson` parses the provider payload — dedup by
   `cardId` (first wins), malformed entries skipped into `errors`. Cards land
   in `cardDb_`'s vector; `byId_`/`byName_` indexes point into it. From this
   moment card pointers are stable for the whole process.
2. `screenManager_.Replace(makeScreen(AppScreen::MainMenu))` — constructs
   `MainMenuScreen(ctx_)` on the stack.
3. The frame loop, once per frame until `WindowShouldClose()` or an empty
   stack:
   - `dt = GetFrameTime()`
   - `imageCache_.PollAndLoad()` — textures requested by widgets finish
     loading here (never on the calling thread's allocation path).
   - `ScreenEvent ev = screenManager_.Top().Update(dt)` — the active screen
     runs its whole logic for the frame.
   - `handleEvent(ev)` — `Replace` swaps the stack top (`makeScreen`); `Quit`
     breaks the loop.
   - `BeginDrawing() → Top().Draw() → EndDrawing()`.

## 3. The main menu — `MainMenuScreen`

Constructs a `KeyboardNav` (ui/widgets) over the fixed item list
`Duel / Deck Editor / Settings / Testing / Quit`, each mapped to an
`AppScreen` id. Up/Down move the cursor; Enter returns
`ScreenEvent::Replace(AppScreen::…)`; App swaps the screen. Quit returns
`ScreenEvent::Quit` → `Run()` exits → `App` destructs → `PlatformContext`
closes the window → `main` returns 0.

## 4. The deck editor — `DeckEditorScreen`

Holds two `KeyboardNav`s (`poolNav_` over `ctx_.cardDb`, `deckNav_` over
`selectedDeck_`) and a `TextInput` search box. Card art loads through
`imageCache_`. Saving writes a deck file (ui/deck/DeckFile) — the duel
seats from `ctx_.selectedDeck_` when non-empty, else the default deck.
Leaving returns to the MainMenu; `selectedDeck_` survives in `App` because
`ctx_` references it.

---

## 5. The duel screen — construction & lifetime

`DuelScreen(ctx_)` (ui/screens/DuelScreen.hpp) owns the entire duel stack:

1. `openjoey::engine::Duel d` — the state object: `field` (the mat),
   `DuelProtocol turn` (phase walk), `Chain chain`, `lp{8000, 8000}`
   (`DuelConfig::START_LP`), `result = Ongoing`.
2. `openjoey::engine::Engine eng{d}` — the constructor wires the LP hooks:
   `resolver_.ctx.damage/gain` lambda-capture the engine so LP effects
   dispatched by the resolver mutate `d_.lp` (the field layer stays
   duel-free).
3. `DuelSetup::loadDefaultDeck` / `buildDecks` — builds `mainA/mainB` and
   `extraA/extraB` (`isExtraDeckMonster()` routes to the Extra Decks), then
   **wires effects**: for every card, `classicEffectsFor(c.name)` (from
   openjoey-engine `action/Catalog.hpp`) appends each `ActionSpec` to
   `c.effects` — the full spec (scope/amount/needsTarget) survives.
4. `DuelSetup::seatDecks` — `engine.setDeck(p, ptrs)` fills
   `field.deckZones[p]` (state.controller/owner set, no location copy —
   placement truth lives in the zones); Extra-Deck cards go into
   `field.extraDeckZones[p]`.
5. `eng.drawOpeningHands()` — `Move_Draw` ×5 per player
   (`DuelConfig::START_HAND`).
6. `eng.startTurn()` — turn 1: `skipDraw = skipBattle = true`
   ("the player who goes first"), phase = Draw.
7. `fieldGrid_.build(field_, viewer)` — **built once per duel** (the zone
   addresses are stable) and rebuilt only on hotseat viewer swap. Rows are
   viewer-relative (`FieldRows.hpp`): 1 opp S/T, 2 opp monsters, 3 own
   monsters, 4 own S/T — monster rows adjacent at the mid-line.

## 6. The duel frame loop — `DuelScreen::Update(float dt)`

Per frame, in order:

1. **Handoff gate** — hotseat pass-the-device screen blocks input until
   confirmed.
2. **Input sampling** — keys are sampled once and dispatched by
   `st.mode` (`DuelMode`: Navigate / Menu / AttackTarget / EffectTarget /
   TributeTarget). All modes share full 2-D cursor movement (the
   once-Navigate-only bug class is gone: movement goes through
   `KeyboardNav2D` on the grid).
3. **Menu** — ENTER on a zone opens the contextual menu:
   `DuelActions::rebuild()` walks the cursor's zone and pushes
   `DuelAction{label, invoke, ActionId tag}` entries — every entry is tagged
   with its `openjoey::ActionId`, 1:1 with the engine vocabulary.
4. **Invoke** — running an action calls **Engine methods only** (e.g.
   `normalSummon`, `tributeSummon`, `equipCard`, `activateEffect`); the UI
   never mutates engine/LP state directly. Costs are charged inside
   `Engine::activateEffect` (once, never refunded). The returned log line is
   shown in the status strip; `st.post()` opens the chain responder window
   when the engine reports a new Chain Link.
5. **Targeting** — AttackTarget/EffectTarget modes pick a card and re-invoke;
   TributeTarget toggles tributes and F confirms (fusion/ritual flows).
6. **Chain window** — R resolves (`eng.resolveChain()`), then
   `fx.sweepResolved()` moves activated spells/traps to the Graveyard.

## 7. The Engine — turn machine (protocol/DuelProtocol.hpp + duel/engine/Turn.hpp)

`DuelProtocol` walks Draw → Standby → Main1 → Battle → Main2 → End
(`canAct()` = Main1/Main2/Battle). `Engine::startTurn()`:

1. `resetPerTurnState()` — once-per-turn sets (`normalSummonUsed_`,
   `attacked_`, `flipSummoned_`, `positionChanged_`) clear; card
   `state.setThisTurn/placedThisTurn` flags expire (this is what lets a set
   trap activate on a later turn).
2. Turn 1: `skipDraw/skipBattle` set (starting player).
3. Otherwise `drawForTurn()` — `Move_Draw(field, p, 1)`; empty deck here =
   `DeckOut` loss (`WinReason::DeckOut`).

`Engine::endTurn()` — hand limit: `discardToHandLimit()` discards down to
`DuelConfig::HAND_LIMIT` (6); the turn swaps, `turnNumber` increments, flags
reset. `toMain1/toBattle/toMain2` enforce the legal phase edges (no Battle on
turn 1, Main2 only after Battle).

## 8. The Engine — the action realization system (the core)

Every action in the game is `openjoey::ActionId` (213, foundation
`action/ActionId.hpp`) realized **twice-over**:

* **Builtins** (`include/action/builtins/`, one HPP per zone-move primitive):
  `Move_Draw`, `Move_MillToGY`, `Move_DiscardToGY`, `Move_DestroyToGY` (with
  the leave-field sweeps), `Move_Banish`, `Move_ReturnHand/Deck`,
  `Move_SearchToHand`, `Move_Excavate`, `Move_MaterialsToGY`, `Summon_ToMMZ/
  Normal/Set/Special/Token/Fusion/Ritual`, `Pos_Flip/Change`, `Equip_Equip/
  Unequip`, `Place_Counter/Remove_Counter`. All expressed as remove/put
  against zones; `detail/LeaveField` guarantees: equips detach with exact
  bonus rollback, a destroyed monster destroys its equips, **tokens cease to
  exist** off the field (their zone slot is cleared and the mat drops
  ownership).
* **One HPP per action** (`include/action/actions/<Name>.hpp`, 213 files):
  `act_<Name>(args)` Engine members — the per-action realizations
  (`act_NormalSummon` → `normalSummon(args.target)`, `act_DiscardUntilHas6` →
  `discardToHandLimit()`, `act_CheckReplay` → `checkReplay()`, …).
* **`Engine::perform(id, args)`** — the exhaustive 1:1 dispatcher; the
  iterate-all-ids test fails the build if any action lacks a verdict.
  Post-classic mechanics (Synchro/Xyz/Pendulum/Link) realize as
  classic-format gates.
* **`activateEffect(spec, activator, args)`** — card effects: Spell-Speed
  legality via `Chain::legalToChain`, `spec.lpCost` charged once (never
  refunded), link pushed onto the chain. `resolveChain()` walks last-link-
  first through the `ActionResolver` (LP effects run through the wired
  `DuelContext`; mass destruction is `TargetScope`-driven — no sentinels);
  negation blanks the responded-to link; Flip effects triggered mid-chain
  resolve as a follow-up chain.

---

## 9. The Engine — summon family & battle (duel/engine/Summon.hpp, Battle.hpp)

**Summon family** (Main Phase, legality owned by the engine):

* `normalSummon(c)` / `normalSet(c)` — once per turn (`canNormalSummon()`),
  `tributesRequired(c)` (Lv5–6 → 1, Lv7+ → 2, `DuelConfig::tributesFor`)
  routes to `tributeSummon`. Placement: `action::Summon_ToMMZ` sets zone
  orientation + visibility (face-down = `Limited`, controller-only).
* `tributeSummon(c, tributes, faceDown)` — tributes → controller's GY, then
  placement; `tributeSet` is the face-down variant.
* `flipSummon(c)` — `canFlipSummon` guards (face-down, yours, not set this
  turn); Flip effects auto-push onto the chain (targeted ones wait for a
  manual target).
* `specialSummon(c, faceDown)` — from ANY zone you own
  (`SpecialSummonFromHand/Graveyard/Banished/ExtraDeck`), position chosen.
* `fusionSummon(extra, materials)` — Extra Deck → first free EMZ,
  transactional (placement validated before materials move);
  `ritualSummon(monster, tributes)` — hand → MMZ, Σ levels ≥ level.
* `summonToken(name, atk, def)` — the mat owns the token
  (`Field::tokens`); it ceases to exist off the field.

**Battle** (`canAttack`: Battle Phase open + your face-up ATK monster +
`!attacked_.count`): `declareAttack(c, target)` **holds the attack open**
(`pending_`, trace `BattleStep::AttackerChosen → TargetChosen/DirectDeclared`),
`checkReplay()`/`confirmAttack()` re-validate (Replay rules — field changed →
cancel), `resolveDamage()` finishes it:

1. Face-down defender flips face-up (visibility only); its Flip effects push
   onto the chain.
2. Math on **`effectiveAtk()/effectiveDef()`** (base + `state.atkMod/defMod`
   from equips/effects, ≥ 0):
   ATK vs ATK — higher destroys lower, pierces the difference; tie destroys
   both. ATK vs DEF — higher destroys (no damage), lower rebounds, equal =
   nothing. Each outcome recorded in `lastDamageOutcome_` (the
   `protocol::DamageOutcome` values mirror the rulebook's damage table).
3. `destroy()` routes through `action::Move_DestroyToGY` — the sweeps run
   (equips destroyed with the host, tokens erased).
4. Direct attacks only vs an empty field; damage = full ATK.
5. `resolveChain()` fires flip effects/responses, then `checkWinConditions()`
   (LP depletion, both-at-0 = Draw; `setResult` decided duels are never
   overwritten).

**Support actions**: `equipCard/unequipCard` (attach + exact bonus
rollback), `placeCounter/removeCounter`, `searchDeck(pred)`, `excavate(n)`,
`setResult(r, WinReason::CardEffectWin)`, `resolveStandby()` (standby
triggers). **AI seams**: `observe(viewer)` returns a visibility-correct
`StateView`; `legalActions(player)` returns the typed action space.

## 10. Rendering a frame — `DuelScreen::Draw()`

`FieldGrid::Draw()` walks `FieldRows` (semantic rows, viewer-relative) and
draws one `ZoneCell` per zone: face-down cards render as card backs
(`Visibility::Limited` shows details to the owner only); labels, LP and
phase come from the engine state; card art comes from `CardImageCache`
(textures already polled in `App::Run`). `ZoneInfoPanel` prints the
cursor's zone summary; `DuelPanels` the chrome (LP, turn, phase, log strip).
No draw call touches engine state.

## 11. The hotseat loop (full turn sequence, values at every step)

1. P1's turn: `turnPlayer = 0`, phase Draw → Main1 (startTurn consumed the
   draw). Handoff screen shows only after sensitive moves.
2. Normal summon: menu → `NormalSummon` tag → `eng.normalSummon(c)` →
   `normalSummonUsed_ = true`, `c->state.placedThisTurn = true`.
3. Enter Battle (`toBattle`), declare attack, resolve — `attacked_` grows,
   `d_.lp` moves, `graveyardZones` fill.
4. End turn: hand limit enforced, `turnPlayer` flips, flags reset.
5. Handoff → P2 repeats with the viewer-relative grid flipped (rows 1–2 are
   always the opponent's side).
6. Win: LP ≤ 0 (both = Draw), deck-out at the mandatory draw, or
   `WinReason::CardEffectWin` via `setResult` — a decided duel is never
   overwritten.

## 12. Rematch and quitting

Rematch constructs a fresh `Duel` + `Engine` (and re-runs §5) — engine state
is fully self-contained, nothing survives. Quit from the main menu returns
`ScreenEvent::Quit`; `Run()` exits; destructors close raylib. `Config::Save`
has already written `user_settings.json` from the Settings screen.

## 13. The other screens (execution notes)

* **SettingsScreen** — edits `ctx_.settings` in place; Save persists
  `user_settings.json` (foundation `Config::Save`) and `applyWindow` re-applies
  window options live.
* **TestingScreen** — sandbox: exercises engine calls headlessly in-UI.
* **DeckEditorScreen** — see §4.

## 14. State quick-reference — who owns what

| State | Owner | Mutated by |
|---|---|---|
| Card definitions, DB indexes | `openjoey::cards::CardDatabase` (app) | parser, at load |
| Card duel state (`CardState`) | the `Card` objects (app-owned storage, referenced by engine zones) | engine only |
| Placement (orientation/visibility) | the zones (`engine::zone`) | engine via `action/builtins` |
| LP, turn, phase, chain, result | `openjoey::engine::Duel` | `Engine` methods only |
| Once-per-turn flags | `Engine` sets | `resetPerTurnState` expires them |
| Tokens | `Field::tokens` (unique_ptr) | mat-owned; erased on leaving |
| Config/settings | `Config` (foundation) | app Settings screen (Save) |
| Screen stack | `ui::ScreenManager` | `ScreenEvent::Replace` |

## 15. Glossary of the value-flow rules

1. **One vocabulary** — player actions and card effects are both
   `ActionSpec(ActionId, …)`; the menu, the chain and the catalog speak it.
2. **Placement truth lives in zones** — cards never carry a location copy;
   `Field::findCard` derives it. It cannot desync.
3. **One dispatch point** — zone mutations: `action/builtins`; card effects:
   `ActionResolver`; player verbs: `perform()`/flow methods. Nothing else
   touches zones.
4. **Costs once, never refunded** — charged at activation in
   `activateEffect`, even if later negated.
5. **The UI never writes engine state** — it calls engine methods and renders
   `observe()`-style reads.
6. **Numbers are configuration** — LP/hand/deck/tribute values live in
   `DuelConfig`; a different ruleset later = a different preset.
7. **Headless is sacred** — foundation/cards/engine/ai never include raylib;
   the entire ruleset is testable without a GPU (and that is what the AI
   trains on).
