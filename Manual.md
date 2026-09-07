# OpenJoey2 — Runtime Manual

The execution order of the program from `main` to exit, traced through the
monorepo. Paths are relative to the repo root; namespaces mirror the paths
(`openjoey`, `openjoey::cards`, `openjoey::engine`, `openjoey::ui`).

---

## 0. Layer map (who runs inside whom)

```
src/main.cpp                 openjoey::app::run(argc, argv)
└─ ui/core/App.hpp           openjoey::ui::App
   ├─ Config::Load(argv0)    include/Config.hpp        layered settings
   ├─ PlatformContext        ui/platform/              raylib window + loop
   ├─ CardDatabase cardDb_   cards/CardDatabase.hpp    parsed cards.json
   ├─ CardImageCache         ui/cards/                 async art (thread + LRU)
   ├─ AppContext ctx_        ui/core/AppContext.hpp    refs to all of the above
   └─ ScreenManager          ui/core/ScreenManager.hpp screen stack
      └─ Run() / tick()      one frame = Update → event → Draw
         └─ DuelScreen       ui/screens/DuelScreen.hpp — owns the duel stack:
            ├─ Engine engine_            engine/duel/Engine.hpp (facade)
            ├─ Duel duel_                engine/duel/Duel.hpp  (state + config)
            ├─ FieldGrid / ZoneCell      ui/duel/              mat rendering
            ├─ DuelActions act_          ui/duel/              legal menus
            ├─ DuelEffects fx_           ui/duel/              activations
            └─ DuelUIState ui_           ui/duel/Action.hpp    mode + log
```

Native: `Run()` loops `tick()` until quit. Web (`__EMSCRIPTEN__`): `tick()` is
registered with `emscripten_set_main_loop_arg` — the browser owns the loop.

## 1. Startup (`DuelScreen::setupDuel`)

1. `DuelSetup::buildDecks` — deck source: editor selection → `data/decks/default.txt`
   → first-40-DB fallback. Fusion-frame cards route to the Extra Deck.
2. `DuelSetup::seatDecks` — decks into zones, `Engine::setDeck` **seals the
   backing pointers** (`deckBackings`), then `shuffleDecks`.
3. Format switches copied from settings → `duel_.config`
   (`chainResponseWindow`, `autoDiscardEndPhase`).
4. `drawOpeningHands` (5 each) → `startTurn` (turn 1: no draw, no Battle Phase)
   → `toMain1`.
5. `fieldGrid_.setViewer(turnPlayer)` — hotseat view belongs to the turn player.

## 2. One player action (state that changes)

Every menu offer is gated by an engine predicate (`Can*` in
`engine/action/State.hpp`) — the menu cannot offer an illegal action.

| Action | Engine entry | State changed |
|---|---|---|
| Normal/Set/Tribute summon | `Engine::normalSummon` … | zone move hand→MMZ, `normalSummonUsed`, tribute→GY |
| Flip summon / position | `flipSummon` / `changePosition` | orientation+visibility, `flipSummoned`/`positionChanged`/`attacked` guards |
| Declare attack | `declareAttack` | `turnState.pending` held open (replay window) |
| Damage | `resolveDamage` | destruction → GY, LP delta, `attacked`, flip → face-up + flip triggers |
| Activate | `activateEffect` | LP cost charged (never refunded), chain link push; set-turn Traps rejected (p.31) |
| Chain response | `passResponse` / `resolveChain` | reverse-order resolution; window mode needs both players to pass |
| End turn | `endTurn` | hand-limit discard, `ResetPerTurnState`, turn swap |
| Undo | `undo` (`Z`) | restores the last checkpoint: zones, LP, chain, `CardState` of every card, token clones |

Every entry point calls `checkpoint()` first — a bounded (30) snapshot stack
built by `cloneDuel` (`engine/duel/Undo.hpp`), which deep-copies engine-owned
tokens (pointer remap via `IZone::replacePtr`) and snapshots external cards'
`CardState`.

## 3. Effect resolution (catalog-driven)

`Card` carries **identity + duel state only** — no effect list. Effects resolve
by name through the classic catalog (`engine/action/Catalog.hpp`):

* activation: the UI looks up `findClassicEffect(name)`, arms
  `pendingFx`, and `ActivateEffect` validates Spell Speed + set-turn Traps;
* flip/standby triggers: the engine iterates `classicEffectsFor(name)` and
  pushes Trigger specs onto the chain;
* resolution: `ResolveChain` runs links last-activated-first; negation blanks
  the responded-to link.

## 4. Format switches (Settings → persisted in user_settings.json)

| Switch | ON (default) | OFF |
|---|---|---|
| Chain response window | chains resolve after **both** players pass (`PassResponse`) | explicit resolve (`R`), legacy auto-resolve of triggers |
| End Phase auto-discard | `EndTurn` discards to 6 automatically | turn is refused until the player discards (`CanEndTurn`) |

Applied per duel in `setupDuel` from `ctx.settings` → `duel_.config`.

## 5. UI affordances

* **Mouse**: click = move cursor + ENTER (menu/confirm/target); right-click =
  cancel; hover = inspect. Hit-testing lives in `FieldGrid::pointToCursor` —
  the click reuses the exact keyboard pathways.
* **Legal-target highlights** (`FieldGrid::addHighlight`): red = attack
  targets, green = effect targets, orange = tribute picks.
* **Phase timeline** in the header; **duel log** overlay (`L`, capped at 200
  lines, chain/damage lines colored).
* **Hotseat handoff**: SPACE gate before each new turn hides the previous
  player's view change.

## 6. Win / loss

`CheckWinConditions` (`engine/action/State.hpp`): LP ≤ 0 (both = Draw),
deck-out at the mandatory draw, card-effect win. Rematch: `R` →
`duel_ = Duel{}` + `hardReset()` + `setupDuel()`.
