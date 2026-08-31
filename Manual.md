# OpenJoey2 — Runtime Execution Manual

This is not an API reference. It is the **execution order of the program**, from
process start (`main`) to process exit, written as a line-by-line, function-by-
function trace. For every stage it states **which values change** — whose state
moves, where pointers land, which enums flip. Follow it top to bottom and you
are literally watching the program run.

Everything below is read from the actual headers/sources in this repo
(header-only architecture: most "functions" live in headers and execute inline
in the order shown). File paths are relative to the repository root.

---

## 0. Map of the runtime layers (who runs inside whom)

Execution is strictly layered. Each layer only calls the one below it:

```
main()                                    openjoey-app/src/main.cpp
 └─ App (ctor → Run)                      openjoey-app/include/ui/core/App.hpp
     ├─ Settings::Load                    openjoey-core/include/openjoey/Settings.hpp
     ├─ PlatformContext (raylib window)   openjoey-app/include/ui/platform/PlatformContext.hpp
     ├─ CardImageCache (bg dl thread)     openjoey-cards/include/openjoey/cards/ui/CardImageCache.hpp
     ├─ App::Run → main loop
     │   ├─ ScreenManager::Top().Update() openjoey-app/include/ui/core/ScreenManager.hpp
     │   │   └─ one of the 5 screens      openjoey-app/include/ui/screens/*.hpp
     │   │       └─ DuelScreen            openjoey-app/include/ui/screens/DuelScreen.hpp
     │   │           ├─ DuelActions       openjoey-app/include/ui/duel/DuelActions.hpp
     │   │           ├─ DuelEffects       openjoey-app/include/ui/duel/DuelEffects.hpp
     │   │           ├─ FieldGrid         openjoey-app/include/ui/duel/FieldGrid.hpp
     │   │           ├─ DuelPanels        openjoey-app/include/ui/duel/DuelPanels.hpp
     │   │           └─ Engine (rules!)   openjoey-gameplay/include/duel/Engine.hpp
     │   │               ├─ Turn.hpp      openjoey-gameplay/include/duel/engine/Turn.hpp
     │   │               ├─ Summon.hpp    openjoey-gameplay/include/duel/engine/Summon.hpp
     │   │               ├─ Battle.hpp    openjoey-gameplay/include/duel/engine/Battle.hpp
     │   │               └─ Effects.hpp   openjoey-gameplay/include/duel/engine/Effects.hpp
     │   │                   └─ EffectResolver  openjoey-gameplay/include/field/EffectResolver.hpp
     │   │                       └─ EffectsBuiltIn openjoey-gameplay/include/field/EffectsBuiltIn.hpp
     │   │                           └─ Field/Zone (lowest layer)
     │   ├─ BeginDrawing → Top().Draw()
     │   └─ EndDrawing
     └─ (loop exit) → destructors → return 0
```

Bottom-of-the-stack value holders (the objects every action above mutates):

| Object | Header | What it stores |
|---|---|---|
| `Card` | openjoey-cards/include/openjoey/cards/Card.hpp | one card: definition (`name, cardId, atk/def/level, effects`) + duel state (`owner, controller, location, position, setThisTurn, placedThisTurn`) |
| `CardDatabase` | openjoey-cards/include/openjoey/cards/CardDatabase.hpp | all parsed cards + `byId_`/`byName_` indexes |
| `IZone` → `Zone` / `ZoneStack` | openjoey-gameplay/include/zone/ | raw `Card*` holders; slots also carry `ori_` (ATK/DEF) and `vis_` (Visible/Limited/Restricted) |
| `Field` | openjoey-gameplay/include/field/Field.hpp | all zones for both players (arrays listed in §11) |
| `Duel` | openjoey-gameplay/include/duel/Duel.hpp | `lp[2], turnPlayer, turn{number,phase}, result, chain, field` |
| `Engine` | openjoey-gameplay/include/duel/Engine.hpp | the rule machine: owns a `Duel&`, `EffectResolver resolver_`, per-turn legality sets, pending attack, battle trace |
| `DuelUIState` (`ui_`) | openjoey-app/include/ui/duel/Action.hpp | screen-only state: `mode, cursorRow/Col, handCursor, actionCursor, selectedZone, pendingCard, tributePicks, handoff, chainPrompt, lastResult…` |

---

## 1. Process start

### 1.1 `main()` — openjoey-app/src/main.cpp

```cpp
int main(int argc, char** argv) {
  openjoey::ui::App app(argc > 0 ? argv[0] : nullptr);
  app.Run();
  return 0;
}
```

Execution order:

1. **Line 7 — construct `App`** (full detail in §1.2). `argv[0]` is passed so
   `Settings` can locate `data/` beside the executable; it does not touch any
   game state.
2. **Line 8 — `app.Run()`** — the entire program life happens here (§3).
3. **Line 9 — `return 0`**: `app`'s destructor runs (member-wise, reverse
   declaration order — see §13), then the process exits.

### 1.2 `App` constructor — ui/core/App.hpp:25-30

Declared members initialize **in declaration order** (this order is load-bearing):

| # | Member | What runs | Values set |
|---|---|---|---|
| 1 | `settings_` | `Settings::Load(argv0)` — reads/creates the JSON settings file next to `data/` | `settings_.screenWidth` (1620), `screenHeight` (920), `targetFps` (60), `fullscreen`, `paths.cardsJson`, `paths.cardImgDir`, `paths.cardImgUrl`, `paths.cardImgSmallUrl` |
| 2 | `appConfig_` | `makeConfig(settings_)` — App.hpp:57-65 copies the four settings into an `AppConfig` | `cfg.screenWidth/screenHeight/targetFps/fullscreen` |
| 3 | `platform_` | `PlatformContext(appConfig_)` — **this is where raylib boots**: `SetConfigFlags` (VSync hint / fullscreen flag), `InitWindow(w, h, "OpenJoey")`, `SetTargetFPS(60)`. On destruction it calls `CloseWindow()` (§13) | OS window created; raylib global state initialized |
| 4 | `cardDb_` | `CardDatabase` default ctor — `cards_, byId_, byName_` empty | still empty |
| 5 | `selectedDeck_` | empty `std::vector<Card>` | empty |
| 6 | `imageCache_` | `CardImageCache(imgDir, imgUrl, imgUrlSmall)` ctor (CardImageCache.hpp:27-34): stores the three paths **and starts `worker_ = std::thread(workerLoop)`** — a background download thread now waits on `cv_` (§10.3) | `imgDir_, imageUrl_, imageUrlSmall_`, `stop_=false`, live worker thread |
| 7 | `ctx_` | aggregate init of `AppContext{cardDb_, selectedDeck_, imageCache_, settings_}` — references only, no copies | `ctx_.cards`, `ctx_.selectedDeck`, `ctx_.imageCache`, `ctx_.settings` now alias the members above |
| 8 | `screenManager_` | empty stack | no screen yet |

No window content has been drawn yet; no cards are loaded yet.

---

## 2. Boot into the first screen

### 2.1 `App::Run()` — ui/core/App.hpp:90-107

```cpp
LoadCards();
screenManager_.Replace(makeScreen(AppScreen::MainMenu));
while (!WindowShouldClose() && !screenManager_.Empty()) { ... }
```

Step by step:

1. **`LoadCards()`** (App.hpp:67-71):
   - `path = settings_.paths.cardsJson` (e.g. `<exe dir>/data/cards.json`).
   - `cardDb_.LoadFromFile(path)` (CardDatabase.hpp:33-60):
     - opens the file, slurps it into a string;
     - `LoadFromString` → `Clear()` → `cards::parseRemoteCardJson(content)`
       (CardParser.hpp:29-74): nlohmann parse; expects `{"data":[…]}`; per entry
       `detail::cardFromRemoteJson` builds a `Card` (sets `cardId`, `imageId =
       cardId`, `name`, `atk/def/level`, `frameType`…); duplicate ids are
       skipped; bad entries go to `errors` and parsing **never aborts**;
     - back in `LoadFromString`: `cards_ = std::move(parsed.cards)` then two
       index loops fill `byId_[c.cardId] = &c` and `byName_[c.name] = &c`
       (first name wins).
   - On failure: `[App] Failed to load …` on stderr; `cardDb_` stays empty and
     every screen later degrades to an "empty DB" hint.
   - **Changed:** `cardDb_.cards_` (N cards), `byId_`, `byName_`.
2. **`screenManager_.Replace(makeScreen(AppScreen::MainMenu))`**:
   - `makeScreen` (App.hpp:73-83) `new`s a `MainMenuScreen(ctx_)`.
   - `ScreenManager::Replace` (ScreenManager.hpp) destroys any current screen
     and pushes the new `unique_ptr<IScreen>`; the stack holds exactly one.
   - **Changed:** `screenManager_.stack_ = [MainMenuScreen]`.
3. **The main loop** — runs once per displayed frame until it exits (§3.4):

| Line | Call | Effect |
|---|---|---|
| 95 | `float dt = GetFrameTime()` | seconds since last frame |
| 96 | `imageCache_.PollAndLoad()` | moves finished background downloads into GPU textures (§10.3) |
| 98 | `ScreenEvent ev = screenManager_.Top().Update(dt)` | **all gameplay logic of the active screen happens here** |
| 99 | `if (ev.type == Type::Quit) break;` | the only in-app exit path (§3.4) |
| 100 | `handleEvent(ev)` | on `Replace`: destroys the current screen and `Replace(makeScreen(ev.target))` — screens are recreated fresh on every navigation |
| 102-105 | `BeginDrawing(); Top().Draw(); EndDrawing();` | the frame is rendered (§12) |

The loop condition `!WindowShouldClose()` makes the OS close button a second
exit path; `!screenManager_.Empty()` is false only if a screen ever popped
itself — it never does in practice.

---

## 3. The main menu — `MainMenuScreen`

openjoey-app/include/ui/screens/MainMenuScreen.hpp. Members: `nav_` is a
`KeyboardNav{cursor=0, count=kItemCount(5)}`, `kItems = {"Duel","Deck
Editor","Settings","Testing","Quit"}`, `kScreenMap = {Duel, DeckEditor,
Settings, Testing, MainMenu}` (MainMenuScreen.hpp:69-72).

### 3.1 Every frame: `MainMenuScreen::Update(float)`

```cpp
nav_.setCount(kItemCount);          // re-clamps cursor into [0,4]
if (nav_.handleWrapKeys()) ...      // KEY_UP/KEY_DOWN → cursor = (cursor±1) mod 5
if (IsKeyPressed(KEY_ESCAPE)) ...   // ignored at top level
if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
    if (nav_.cursor == kItemCount - 1)      // cursor==4 ("Quit")
        return ScreenEvent::quit();          // {Type::Quit} → App::Run breaks
    return ScreenEvent::replace(kScreenMap[nav_.cursor]);
}
return ScreenEvent::none();
```

**Values changed per frame:** `nav_.cursor` on Up/Down. On Enter:
`ScreenEvent.type/target` — the *return value* is the entire communication.

### 3.2 Every frame: `MainMenuScreen::Draw()`

Draws the title, the 5 items with `>` before `kItems[nav_.cursor]`, and the
help footer. Reads state only; mutates nothing.

### 3.3 Navigation decision (App::handleEvent, App.hpp:85-88)

- `Replace(Duel)` → `makeScreen` constructs `DuelScreen(ctx_)`, old menu screen
  is destroyed, duel becomes `Top()` — continue at §5.
- `Replace(DeckEditor)` → `DeckEditorScreen(ctx_)` — continue at §4.
- `Replace(Settings|Testing)` → trivial screens; ESC returns
  `ScreenEvent::replace(AppScreen::MainMenu)` and the cycle restarts here.
- `Quit` → `App::Run` breaks; teardown in §13.

---

## 4. The deck editor — `DeckEditorScreen` (the bridge into a duel)

openjoey-app/include/ui/screens/DeckEditorScreen.hpp. This is the only screen
that fills `ctx_.selectedDeck`, which the duel later consumes.

Members (constructed at screen creation):
`pool_` = **copy** of `ctx_.cards.GetAllCards()`, sorted once by
`ui::sortPool(pool_, DeckSortMode::Type)` (DeckFilters.hpp:45-58);
`deck_` (empty vector<Card>); `poolNav_`/`deckNav_` (`KeyboardNav`, clamp mode);
`searchInput_` (`TextInput`, text `"Right click to search..."`, `typing_=false`);
`focusPool_=true`; `deckGridView_=false`; `statusMsg_`="";
`preview_` (`CardPreview`, `card_=nullptr, scrollLines_=0`).

### 4.1 `Update()` per frame — DeckEditorScreen.hpp:~60-182

Order inside the function:

1. `if (IsKeyPressed(KEY_ESCAPE) && !searchInput_.isTyping())`
   **return** `ScreenEvent::replace(AppScreen::MainMenu)` — back to §3, deck is
   discarded (deck_ dies with the screen).
2. `searchInput_.Update()` (TextInput.hpp:25-44):
   - right mouse press toggles `typing_` and clears `text_`;
   - while typing: pumps `GetCharPressed()` into `text_` (printable chars only),
     BACKSPACE pops one char, ESC ends typing; sets `changed_`.
3. `filterPool(pool_, typeFilter_, searchInput_.GetText())`
   (DeckFilters.hpp:61-79) → vector of pointers into `pool_` matching the type
   filter + case-insensitive substring. Rebuilt **every frame**; `poolSz` set.
4. **Pool focus** (`focusPool_ == true`):
   - `poolNav_.setCount(poolSz); poolNav_.handleClampKeys();`
     → `poolNav_.cursor` clamps 0..poolSz-1 on Up/Down (+PgUp/PgDn step 10).
   - **ENTER** on a card (DeckEditorScreen.hpp:114-125): legality check
     `(int)deck_.size() < 60` and `countCopies(deck_, card.cardId) < 3`
     (DeckFilters.hpp:81-86); if legal `deck_.push_back(card)` (a **copy** of
     the pool card) and `statusMsg_ = "Added: <name>"`; otherwise the reason.
     *Changed: `deck_` size +1, `statusMsg_`.*
   - TAB/RIGHT → `focusPool_ = false`.
5. **Deck focus** (`focusPool_ == false`):
   - `deckNav_` Up/Down (grid view: step = 4 columns), DELETE/BACKSPACE/D
     removes `deck_[deckNav_.cursor]` (erase; `statusMsg_ = "Removed: …"`),
     G toggles `deckGridView_`, TAB/LEFT → back to pool.
     *Changed: `deckNav_.cursor`, maybe `deck_`.*
6. **S** → `SaveDeck("default")` → `ui::DeckFile::Write(path, deck_)`
   (DeckFile.hpp:18-23): creates `settings_.paths.deckDir`, writes one
   `cardId` per line.
7. **L** → `LoadDeck("default")` → `DeckFile::Read(path, ctx_.cards, 60)`
   (DeckFile.hpp:27-44): reads ids, resolves each via `db.GetCardById(id)`,
   skips unknown ids/comments, caps at 60 → rebuilds `deck_`.
8. **ENTER on deck focus with deck ≥ 40 cards** (DeckEditorScreen.hpp:170-173):

```cpp
ctx_.selectedDeck = deck_;                    // copy deck into AppContext
return ScreenEvent::replace(AppScreen::Duel); // → §5
```

**Changed:** `ctx_.selectedDeck` now holds the 40-60 chosen cards; the screen
object is destroyed by `Replace` next frame.

### 4.2 `Draw()` per frame

Left pool list/grid (uses `poolNav_.cursor`), right deck list (or 4-column
grid), `preview_.SetCard(&hovered card)` + `preview_.Draw(bounds,
ctx_.imageCache)` for whichever card the cursor is on, status line, deck
stats. Pure rendering.

---

## 5. The duel screen — construction & lifetime

openjoey-app/include/ui/screens/DuelScreen.hpp.

### 5.1 Member construction order (DuelScreen ctor)

| # | Member | Initialized as |
|---|---|---|
| 1 | `duel_` | default `Duel`: `lp[2] = {8000,8000}`, `turnPlayer = 0`, `turn.number = 0`, `turn.phase = Phase::Draw`, `result = DuelResult::Ongoing`, `chain` empty, `field` default (all zones empty) |
| 2 | `engine_` | `Engine(duel_)` — stores `Duel& d_` + `EffectResolver resolver_{d_.field}` + empty legality sets: `summoned_, setThisTurn_, flipSummoned_, positionChanged_, attacked_` all empty; `normalSummonUsed_=false`; `pending_ = {}`; `battleStep_=Idle`, `damageStep_=None`, `lastDamageOutcome_=None`; `battleTrace_` empty |
| 3 | `ui_` (`DuelUIState`) | `mode = Navigate`, `viewer_ = 0`, `cursorRow = fieldRow(OwnMonster)=3`, `cursorCol = 4`, `handCursor = 0`, `actionCursor = 0`, `selectedZone = nullptr`, `pendingCard/pendingTarget = nullptr`, `tributePicks` empty, `tributeCount = 0`, `fusionPending = ritualPending = false`, `handoff = true` **(game starts gated on the pass-device screen)**, `chainPrompt = false`, `helpOpen = false`, `lastResult` = "" |
| 4 | `grid_` (`FieldGrid`) | `ROWS=6, COLS=9`; `viewer_=0`; then `grid_.build(duel_.field)` (FieldGrid.hpp:41-100) fills the 6×9 matrix of `IZone*` + labels: row 1 = opp S/T row (mirrored, deck/extra-deck at the flanks), row 2 = opp monsters + banish/GY/field, row 3 = own monsters + field/GY/banish, row 4 = own S/T row + extra deck/deck; rows 0 and 5 are the hand strips; then `grid_.syncFromField(duel_.field)` — every `ZoneCell` gets `card = zone->peek()`, `facedown = !zone->isVisible()`, `count = zone->count()`, orientation & visibility flags |
| 5 | `actions_` | `DuelActions(engine_, duel_, duel_.field, grid_, fx_, ui_)` — references only |
| 6 | `fx_` | `DuelEffects(engine_, duel_.field, ui_)` — references only |
| 7 | `panels_` | `DuelPanels` (stateless) |
| 8 | `info_` | `ZoneInfoPanel` (stateless) |

### 5.2 `enter()` — the pre-duel setup (DuelScreen.hpp:~95-120)

Called by `ScreenManager` right after construction. Execution:

1. **Decks** — if `ctx_.selectedDeck` has ≥ 40 cards, both players get **the
   same deck**: `field.deckZones[p]` receives a `Card` copy per deck entry
   (`owner=p, controller=p, location=Location::Deck`), 40-60 cards each.
   Otherwise a small fallback deck is built from the catalog
   (`findClassicEffect` names) so the duel always runs.
   *Changed: `deckZones[0]`, `deckZones[1]` now hold Card objects.*
2. **Shuffle** — `deckZones[p].shuffle()` (ZoneStack.hpp:69,
   `std::shuffle` with a `std::mt19937` seeded by default random device) —
   deck order becomes random. *(P1's and P2's shuffles are independent.)*
3. **Opening hands** — `engine_.drawCards(0, 5); engine_.drawCards(1, 5);`
   → per call `Move_Draw(field, p, 5)` (EffectsBuiltIn.hpp:69-80): 5×
   `deckZones[p].moveTo(handZones[p])` (IZone::moveTo: top card `remove()` +
   `dest.put()`, rollback if the destination rejects) and the drawn card's
   `location = Location::Hand`.
   *Changed: deck count −5, hand count +5, 10 Cards' `location`.*
4. **`engine_.startDuel()`** (Turn.hpp) — see §6.1; after it returns:
   `turn.number = 1`, `turn.phase = Phase::Draw`, `turnPlayer = 0`.
5. **`ui_.handoff = true`** — the pass-device gate is shown; both hands are
   hidden until SPACE (hotseat privacy).

### 5.3 `rematch()` — DuelScreen.hpp:~120-135 (KEY_R after game over)

1. `duel_ = Duel{}` — **the entire Duel object is re-defaulted**: LP 8000/8000,
   `turnPlayer=0`, `phase=Draw`, `result=Ongoing`, fresh empty `field` (all
   `Card` copies from the previous duel are destroyed here).
2. Decks are refilled and reshuffled exactly as in §5.2.1-2, hands redrawn.
3. `engine_.hardReset()` (Effects.hpp:78-84): `resetPerTurnState()` (clears the
   five legality sets + `normalSummonUsed_`), `battleStep_=Idle`,
   `damageStep_=None`, `lastDamageOutcome_=None`, `battleTrace_.clear()`.
4. `engine_.startDuel()`; `ui_` resets to `handoff=true, mode=Navigate,
   chainPrompt=false`; play resumes.

---

## 6. The duel frame loop — `DuelScreen::Update(float dt)`

This is the heart of the program: every keypress the players make flows
through this one function, once per frame. Top-down order of its guards
(each `return ScreenEvent::none()` below means "consume the whole frame,
stay in the duel — a duel never navigates away"):

```cpp
1  grid_.syncFromField(duel_.field);   // cells ← zones: card pointers, facedown, counts
2  if (duel_.result != DuelResult::Ongoing) {
3      if (IsKeyPressed(KEY_R)) rematch();      // §5.3
4      return ScreenEvent::none(); }            // game over: nothing else is live
5  if (ui_.handoff) {
6      if (IsKeyPressed(KEY_SPACE)) {
7          ui_.handoff = false;
8          ui_.lastResult = engine_.beginTurn();  // the turn actually starts (§6.1)
9          ui_.viewer_  = duel_.turnPlayer; }     // view flips to the new player
10     return ScreenEvent::none(); }              // gate frame: rest of input dead
11 if (ui_.chainPrompt) {
12     if (IsKeyPressed(KEY_R)) {                 // nobody chained
13         ui_.lastResult = engine_.resolveChain();   // §8.3
14         fx_.sweepResolved();                       // activated S/T → GY (§7.2)
15         ui_.chainPrompt = false;
16         ui_.mode        = DuelMode::Navigate; }
17     return ScreenEvent::none(); }              // while a chain is open, only R
                                                  // or "chain a set card" works
18 if (ui_.helpOpen) { if (IsKeyPressed(KEY_H)) ui_.helpOpen = false;
19     return ScreenEvent::none(); }
20 if (IsKeyPressed(KEY_H)) { ui_.helpOpen = true; return ScreenEvent::none(); }
21 // navigation (Navigate mode):
22     up/down/left/right → grid_.move(...)  or hand-row scroll (handCursor)
23     IsKeyPressed(KEY_ENTER) → ui_.mode = DuelMode::Menu; actions_.rebuild();
24 // phase shortcuts (Navigate mode): B → toBattlePhase(), N → toMain2(),
25     E → endTurnFlow();  R → resolve chain (same as line 12-16)
26 // Menu mode: UP/DOWN move ui_.actionCursor; ESC → mode=Navigate;
27     ENTER runs actions_[ui_.actionCursor].run() → ui_.lastResult = result,
       mode=Navigate
28 // AttackTarget mode: ENTER confirms target → engine_.confirmAttack();
       ESC → engine_.cancelAttack() + mode=Navigate
29 // EffectTarget mode: ENTER picks target card → fx_.finishActivation(target)
30 // TributeTarget mode: ENTER toggles picks (actions_.toggleTributePick),
       F → actions_.enterTributeConfirm(), ESC cancels (mode=Navigate,
       pendingCard=nullptr)
31 return ScreenEvent::none();
```

Correspondence to the real file: lines 149-152 = game over; 153-159 = handoff;
161-167 = chain window; 169-172 = help modal open; 173 = H opens help;
175-186 = navigation + ENTER; 188-193 = phase keys (`B`/`N`/`E` in the Navigate
branch); 195-307 = the `switch (ui_.mode)` over Menu / AttackTarget /
EffectTarget / TributeTarget, ending with the default `return
ScreenEvent::none()` (DuelScreen.hpp:307).

**Values that can change on any given frame:** `grid_` cell mirrors, then
exactly one of: `ui_.handoff/viewer_`, `ui_.chainPrompt/mode`, `ui_.helpOpen`,
`ui_.cursorRow/cursorCol/handCursor/actionCursor/mode`, `ui_.lastResult`,
and — through the action lambdas — the whole engine/field/duel state (§7-§9).

### 6.1 `endTurnFlow()` (DuelScreen.hpp, called on KEY_E)

```cpp
ui_.lastResult = engine_.endTurn();   // §9 — swaps players, +1 turn, Draw phase
ui_.handoff    = duel_.result == DuelResult::Ongoing;  // gate for the next player
ui_.mode       = DuelMode::Navigate;
```

If the hand limit check inside `endTurn` refuses (`"discard down to 6"`), the
turn does **not** end; `handoff` stays false and the current player must
discard (the engine auto-discards the oldest cards when forced — see §9.3).

---

## 7. The Engine — turn machine & state (openjoey-gameplay)

`Engine` (duel/Engine.hpp) is one class assembled from five headers:
Engine.hpp (state + LP/win helpers) + engine/Turn.hpp + engine/Summon.hpp +
engine/Battle.hpp + engine/Effects.hpp, all included **inside the class body**
as inline member definitions.

### 7.1 Engine state (mutated as the duel runs)

```cpp
Duel&            d_;                 // the duel being driven
EffectResolver   resolver_;          // bound to d_.field
rules::BattleStep battleStep_ = Idle;    // Idle→Start→AttackTarget→Damage→Resolved
rules::DamageStep damageStep_  = None;    // None→Begin→Calculate→Apply→End
rules::DamageOutcome lastDamageOutcome_ = None;
PendingAttack    pending_;           // {attacker, target, direct} during an attack
std::vector<std::string> battleTrace_;
// per-turn legality (cleared by resetPerTurnState() every turn):
std::set<Card*> summoned_, setThisTurn_, flipSummoned_, positionChanged_, attacked_;
bool normalSummonUsed_ = false;
```

### 7.2 `startDuel()` — duel/engine/Turn.hpp

```cpp
turnPlayer = 0;  turn.number = 1;  turn.phase = Phase::Draw;
resetPerTurnState();      // clears the sets + normalSummonUsed_ (nothing to clear)
battleStep_/damageStep_ ← Idle/None; battleTrace_.clear();
```

*Changed: `turn.number 0→1`, `phase Draw`, `turnPlayer 0`.*

### 7.3 `beginTurn()` — Turn.hpp (called at the SPACE gate, §6 line 8)

1. `resetPerTurnState()` — `summoned_/setThisTurn_/flipSummoned_/
   positionChanged_/attacked_.clear()`, `normalSummonUsed_ = false`,
   every `Card::setThisTurn/placedThisTurn` flag on the field is cleared via
   the zone walk (so "cannot Flip Summon the turn it was Set" etc. reset).
2. `drawCards(turnPlayer, 1)` → `Move_Draw(field, turnPlayer, 1)`:
   deck top → hand, card's `location = Location::Hand`.
   *(Turn-1 draw skip for the starting player is applied inside — first turn
   of the duel does not draw.)*
3. `turn.phase = Phase::Draw` (already there), message "player N draws".
   *Changed per turn start: all five legality sets emptied, per-card turn flags
   cleared, hand +1 card, deck −1.*

### 7.4 Phase transitions (Turn.hpp)

- **`toStandbyPhase()`** — legality `phase==Draw`; sets
  `turn.phase = Phase::Standby`. No standby effects exist in the classic set.
- **`toMain1()`** — `phase==Standby` → `turn.phase = Phase::Main1`.
- **`toBattlePhase()`** (KEY_B) — requires `phase==Main1` (or Main2 re-entry);
  sets `turn.phase = Phase::Battle`, `battleStep_ = Start`. The starting
  player cannot enter Battle on turn 1.
- **`toMain2()`** (KEY_N) — from Battle → `turn.phase = Phase::Main2`.
- **`endTurn()`** — §9.

### 7.5 LP & win plumbing (Engine.hpp core)

- `damage(int player, int n)`: `d_.lp[player] -= n` (clamped ≥ 0) →
  `checkWinConditions()`.
- `gain(int player, int n)`: `d_.lp[player] += n`.
- `lp(int p)`: read accessor used by the header UI each frame.
- **`checkWinConditions()`**: if `lp[0]==0 && lp[1]==0` →
  `result = DuelResult::Draw`; else `lp[p]==0` → `result = p==0 ?
  Player1Win : Player0Win` (0-indexed player 0 is shown as "PLAYER 1").
  **This single call is what ends the game**; it runs at the tail of damage,
  chain resolution, and battle resolution. *Changed: `duel_.result` — after
  which `DuelScreen::Update` freezes all input except R = rematch (§6).*
- `drawCards(p, n)`: legality (duel ongoing) + `Move_Draw`; deck-out is **not**
  an instant loss in this build (drawing 0 cards just logs).

---

## 8. The Engine — the summon family (duel/engine/Summon.hpp)

All of these are reached from the action menu: ENTER on a hand card →
`actions_.rebuild()` → menu item → lambda → engine call. The menu side first:

### 8.1 Menu construction — `DuelActions::rebuild()` (DuelActions.hpp:36-47)

Called when ENTER opens the menu (Navigate → Menu). `clear()` empties
`ui_.actions` and resets `actionCursor=0`, then:

- cursor on the **own hand row** with a card → `handActions(c)`:
  - **Monster** (`handMonsterActions`): if `engine.canNormalSummon()`
    (duel ongoing ∧ !normalSummonUsed_ ∧ phase Main1/Main2):
    - `tributesRequired(c) == 0` (Lv ≤ 4): push *"normal summon (ATK)"* →
      `engine.normalSummon(c)` and *"normal set (face-down DEF)"* →
      `engine.normalSet(c)`;
    - Lv 5-6 / 7+: push *"tribute summon — needs N"* whose lambda **does not
      summon yet** — it sets `st.tributePicks.clear(), st.tributeCount = tr,
      st.pendingCard = c, st.mode = DuelMode::TributeTarget`.
  - **Spell/Trap** (`handSpellTrapActions`): look up `findClassicEffect(name)`
    (ClassicCatalog.hpp:88-92, lower-cased name → `ClassicEffect{id, speed, n,
    costLP, needsTarget, note, tp, perOpp}`):
    - `Summon_Fusion` (Polymerization): sets `st.fusionPending = true,
      tributeCount = 2, mode = TributeTarget` (material pick, F to confirm);
    - `Summon_Ritual` (Black Illusion Ritual): `st.ritualPending = true,
      mode = TributeTarget`;
    - any other wired effect: `pushActivate(c, e)` — arms
      `st.pendingCard = c, st.pendingFx = *e, st.pendingOwner = viewer`,
      then **if `e->needsTarget`** → `mode = EffectTarget` ("pick a target"),
      **else** fires `fx_.finishActivation(nullptr)` immediately (§8.6);
    - always pushes *"set in spell/trap zone"* → `fx_.setSpellTrap(c)`.

Every `push(label, fn, rules::RuleAction)` appends a `DuelAction{label, fn, id}`
to `ui_.actions` (Action.hpp). ENTER in Menu mode runs
`ui_.actions[ui_.actionCursor].run()` → `ui_.lastResult = <engine string>`.

- cursor on a **field zone** → `fieldActions(z, me)`:
  - own face-up monster in Battle phase with `engine.canAttack(mc)`:
    *"declare attack"* → `st.attacker = mc; st.mode = AttackTarget`
    (§9.1); in Main phase: *"flip summon"* (face-down) → `engine.flipSummon(mc)`
    or *"switch position (1/turn)"* (face-up) → `engine.changePosition(mc)`;
  - an occupied S/T zone with a wired effect: chain-push during
    `st.chainPrompt` (any set S/T may chain → `engine.activateEffect(...)`,
    and on success `st.activated.push_back(mc)`), or *"activate"* from the
    zone in Main phase via `pushActivate`.

### 8.2 `normalSummon(Card* c)` — Summon.hpp:20-34

1. Guards: card exists ∧ is monster; `canNormalSummon()`; 
   `tributesRequired(c) == 0` else "use tributeSummon".
2. `Summon_ToMMZ(d_.field, c, turnPlayer, faceDown=false)`
   (EffectsBuiltIn.hpp): finds the card's zone (must be the hand),
   `field.firstEmptyMonsterZone(turnPlayer)` (Field.hpp:131-136, first index
   with `isEmpty()`), `z->remove(c)` → `dest.put(c)`; sets orientation
   Vertical (ATK) + visibility Visible; `c->location = Location::Field`.
3. **Values changed:** `normalSummonUsed_ false→true`; card leaves
   `handZones[p]`, lands in `monsterZones[p][slot]`; `c->location` Hand→Field,
   `c->placedThisTurn = true`, zone now Vertical/Visible. Hand −1, monster row +1.

### 8.3 `normalSet(Card* c)` — Summon.hpp:36-49

Same flow with `faceDown=true`: zone orientation Horizontal (DEF) +
visibility **Limited** (only the controller knows it); `c->setThisTurn = true`
(blocks Flip Summon this turn); message "sets a monster" (name hidden).

### 8.4 `tributeSummon(Card* c, vector<Card*> tributes, bool faceDown=false)`
— Summon.hpp:53-79 — reached via `DuelActions::enterTributeConfirm()`
(DuelActions.hpp:56-68) when F is pressed in TributeTarget mode and
`tributePicks.size() == tributeCount`:

1. Guards: monster, `canNormalSummon()`, `tributes.size() == tributesRequired(c)`
   (Lv5-6→1, Lv7+→2), each tribute is the turn player's own field monster.
2. For each tribute: `Move_DestroyToGY(field, t)` (EffectsBuiltIn.hpp) —
   `findCard` → `remove` → `graveyardZones[t->owner].put` →
   `t->location = Location::Graveyard`.
3. `Summon_ToMMZ(..., faceDown)` places the summoner as in §8.2/8.3.
4. **Values changed:** tribute cards MonsterZone→Graveyard (+`location`),
   summoned card Hand→MonsterZone, `normalSummonUsed_ = true`,
   `placedThisTurn`/`setThisTurn` set, `st.pendingCard = nullptr`,
   `mode = Navigate`, `tributePicks` cleared.

### 8.5 `fusionSummon` / `ritualSummon` — Summon.hpp:82-115

After material picking (`toggleTributePick` toggles hovered own monsters
in/out of `st.tributePicks`, cap = 2 for fusion, ∞ for ritual) and F →
`fusionPickMenu()`/`ritualPickMenu()` list the Extra-Deck fusions / hand
rituals; choosing one runs `engine.fusionSummon(f, st.tributePicks)` /
`engine.ritualSummon(m, st.tributePicks)`:

- **fusionSummon** validates both materials + the target is in the owner's
  Extra Deck, then resolver case `Summon_Fusion` (EffectResolver.hpp:150-157):
  `Summon_Fusion(field, c)` (EffectsBuiltIn.hpp:276-296: ED → first free
  **Extra Monster Zone**, Vertical/Visible, `location = Field`) followed by
  `Move_MaterialsToGY(field, a.materials)` (both tributes → their owners'
  GYs) — transactional: placement first, materials only after success.
- **ritualSummon** costs `e.costLP` (2000) via `engine.damage(activator, 2000)`
  then resolver case `Summon_Ritual`: `Summon_Ritual(field, c, tp)`
  (EffectsBuiltIn.hpp:300-320: Hand → first free Main Monster Zone,
  Vertical/Visible) + materials → GY.
- **Values changed:** the fusion monster moves ExtraDeck→EMZ
  (`extraMonsterZones[0..1]`), `location = Field`, controller = summoner;
  ritual monster Hand→MonsterZone; tributes → Graveyards; LP may drop
  (ritual); `fusionPending/ritualPending` back to false; `mode = Navigate`.

### 8.6 Spell/trap activation — `DuelEffects::finishActivation()`
(DuelEffects.hpp:43-59)

After `pushActivate` armed `pendingCard/pendingFx/pendingOwner` (and,
for targeted effects, the player picked a card in `mode == EffectTarget`),
the UI-side call chain is: `fx.argsFor(*e, owner)` builds the resolver args
(scale `n` by the opponent's monster count for `perOpp` effects like Just
Desserts; default `targetPlayer` from the effect id), then
`finishActivation(target)`:

1. `if (st.pendingFx.costLP > 0) engine.damage(st.pendingOwner, costLP)`
   — **LP drops immediately at activation** (cost, not resolution).
2. `engine.activateEffect(id, owner, speed, a)` (Effects.hpp:7-22) —
   guard `chain.legalToChain(speed)` (Spell Speed 1 can never be Chain
   Link 2+), then `d_.chain.push(id, activator, speed, args)` — **the chain
   grows to N links; nothing resolves yet**.
3. On success: a hand spell is seated via `setSpellTrap(c)` (Hand → first
   empty S/T zone, face-down, `location = Field`), `st.activated.push_back(c)`,
   `st.chainPrompt = true` (**the responder window opens**), `mode = Navigate`.
4. The opponent chains via the S/T zone menu, or the activator presses **R**
   → `engine.resolveChain()` (§8.8). After resolution
   `DuelEffects::sweepResolved()` (DuelEffects.hpp:77-84) sends every parked
   activated spell/trap from its S/T zone to `graveyardZones[controller]`
   via `effect_detail::moveCard` and clears `st.activated`.

### 8.7 `flipSummon` / `changePosition` — Summon.hpp:118-188

- **flipSummon(c)** (menu on a face-down own monster, Main phase): guards
  (zone face-down, `!c->setThisTurn`) → `mz->flip()` (Zones.hpp:28-35:
  Horizontal+Limited → **Vertical+Visible**) → `flipSummoned_.insert(c)` →
  any Trigger effect on the card pushes onto the chain (targeted flips are
  skipped — the player resolves them by hand) → `resolveChain()` →
  "... Flip effect triggers." **Values:** zone orientation/visibility flipped,
  `flipSummoned_` grows, flip effects may fire (e.g. Man-Eater Bug's
  500 damage).
- **changePosition(c)**: guards (face-up, not the turn it arrived, not
  already changed) → `mz->changeOrientation(Vertical↔Horizontal)` →
  `positionChanged_.insert(c)`. **Values:** ATK↔DEF orientation flips,
  `positionChanged_` grows.

### 8.8 The chain — `resolveChain()` → `resolveChainImpl(depth)` (Effects.hpp:68-171)

`resolveChain()` sets `chain.step = Resolving`, calls
`resolveChainImpl(0)`, and marks `chain.step = Resolved` when `links` empties.
`resolveChainImpl` loops the links **backwards (last-in, first-out)**:

- `NegateActivation/NegateEffect`: walk to the nearest earlier non-negated
  link and set `links[j].negated = true` (Solemn Judgment) — that link will
  later "resolve without effect".
- `LP_Damage` / `LP_Gain` / `Cost_PayLP`: the engine mutates **`d_.lp`**
  directly through `damage(tp, n)` / `gain(tp, n)`.
- `Summon_Flip`: `resolver_.apply(...)` flips the target face-up and
  remembers it in `flipped`.
- everything else: `resolver_.apply(l.id, l.activator, l.args)` — the single
  dispatch table `EffectResolver::apply` (EffectResolver.hpp:30-172), whose
  cases are pure zone moves: `Move_Draw`/`Move_MillToGY`/
  `Move_DiscardToGY` (EffectsBuiltIn.hpp:69-106), mass destruction
  (`targetPlayer` −1 opp monsters / −2 all monsters / −3 all S/T / −4
  opp ATK-position monsters — Raigeki, Dark Hole, Heavy Storm, Mirror
  Force), banish, return-to-hand, position change.
- then `d_.chain.clear()` → **`checkWinConditions()`** → any cards flipped
  by `Summon_Flip` push their own Flip effects (`deriveFlipArgs`
  auto-targets: damage hits the controller's opponent, destroy/return
  auto-picks the opponent's first monster) and the chain re-runs at
  `depth + 1` (max 4).
- **Values changed:** LP values, `chain.links` emptied, `chain.step`,
  card locations everywhere (GY / hand / banished), zone visibilities.

---

## 9. The Engine — battle (duel/engine/Battle.hpp)

### 9.1 Attack declaration — `DuelScreen` keyboard → `engine.declareAttack`

In Navigate mode with a selected own face-up ATK monster (Battle phase),
**SPACE** → `st.mode = DuelMode::AttackTarget` + hint "pick an opponent
monster (ENTER), or the empty row for a direct attack". ENTER on an occupied
opponent monster zone → `st.pendingTarget = target; runPendingAttack()`;
on an empty opponent zone → `st.pendingDirect = true; runPendingAttack()`.

`runPendingAttack()` (DuelScreen.hpp:293-300) →
`engine_.declareAttack(st.attacker, st.pendingTarget, st.pendingDirect)`
(Battle.hpp:23-49):

1. `guardAttack(a, target)` — turn player owns `a`, phase == Battle,
   `a` in a monster zone face-up ATK, `!attacked_.count(a)`;
   then the target is an opposing monster, or *direct* is only legal with
   `field.countMonsters(1-ap) == 0`.
2. `pending_ = {a, target, direct}`; `battleStep_ = Start`.
3. `confirmAttack()` (Battle.hpp:51-74) — the **replay check** (rulebook
   p.37): returns false if the attacker or the target is no longer on the
   field in Attack position (e.g. something left mid-attack), else
   `battleStep_ = Confirm`.
4. If `confirmAttack()` fails the attack is dropped ("replay!");
   `runPendingAttack()` clears `attacker/target/direct` and
   `mode = Navigate` on success. **Values:** `pending_` now holds the
   attack; nothing else has changed yet.

### 9.2 `resolveDamage()` — Battle.hpp:82-140

Called by `runPendingAttack()` right after `declareAttack`:

1. `confirmAttack()` again (replay gate) → `attacked_.insert(a)` (**the
   attack is now committed; this is the once-per-turn lock**) →
   `battleStep_ = DamageBegin`, `damageStep_ = Calculate`.
2. **Direct:** `damageStep_ = Apply`, `lastDamageOutcome_ = DirectHit`,
   `damage(opp, a->atk)` (**LP − attacker ATK**), `pending_ = {}`,
   `battleStep_ = End`, `checkWinConditions()` → "... attacks directly for N."
3. **Monster vs monster:** `battleStep_ = Confirm`, `damageStep_ = TargetSelected`;
   a face-down defender is flipped via `mz->flip()` (its Flip effects are
   queued as a follow-up chain) → `damageStep_ = Calculate` → classic math:

   - **ATK vs ATK**: higher destroys the lower and pierces the difference;
     a tie destroys **both**; nobody takes damage on a tie.
   - **ATK vs DEF**: higher ATK destroys the defender, **no damage**; lower
     ATK rebounds the difference to the **attacker**; equal → nothing.
   - Destruction goes through `Move_DestroyToGY(field, c)` — the loser(s)
     leave `monsterZones` and land in `graveyardZones[owner]`,
     `c->location = Graveyard`; piercing damage calls `damage(...)`.
4. `pending_ = {}`, `damageStep_ = End`, `battleStep_ = Resolved`,
   `lastDamageOutcome_` set, `checkWinConditions()`.
5. **Values changed per battle:** LP (one or both players), the loser's zone
   (now empty), the GY stack (+1 card), `attacked_`, `pending_` emptied,
   `battleTrace_` records `Start → DamageBegin → Calculate → Resolved`.

Win checks after every damage event: `damage(p, n)` (Duel.hpp) drops
`lp[p]` (floor 0) and sets `result = Player0Win / Player1Win / Draw` when a
player hits 0 (draw → both at 0). While `result != Ongoing`,
`DuelScreen::Update` early-outs after turn 1 of `Update` (the first branch,
DuelScreen.hpp:149-152) — **SPACE/ENTER do nothing; only R works**.

### 9.3 Phase shortcuts — `toBattlePhase()` / `toMain2()` / `endTurn()`

`B` → `engine_.toBattlePhase()` (allowed only in Main1, first turn of the
duel can't enter Battle), `N` → `engine_.toMain2()` (Main1→Main2),
`E` → `endTurnFlow()` →
`engine_.endTurn()` (TurnManager.hpp:26-44):

1. End Phase hand limit (p.45): `if (handZones[cur].count() > 6)`
   **the turn does not end** — `phase = End` and the caller must discard;
   the DuelScreen enforces this by ignoring E until ≤ 6 cards remain.
2. Else `resetPerTurnState()` (TurnManager.hpp:46-54): empties
   `normalSummonUsed_` (fresh Normal Summon/Set), `attacked_`,
   `flipSummoned_`, `positionChanged_` and clears
   `placedThisTurn`/`setThisTurn` on every field card.
3. `turnPlayer = 1 - turnPlayer`, `turnCount++`, `phase = Draw` —
   and the DuelScreen immediately sets `ui_.handoff = true` (hotseat gate).

---

## 10. Rendering a frame — `DuelScreen::Draw()`

Layout math (DuelScreen.hpp:311-345): `_SW/_SH` come from raylib every frame
(StyleSheet.hpp:8-9), so all values below scale with the window:
header `0, 0, _SW, 0.042*_SH` · mat `0, hdr, _SW*0.58, rest` · left panel
`_SW*0.25, hdr, ...` · right panel `x=_SW*0.75, w=_SW*0.25` · footer strip.
Then, in order:

1. `DuelPanels::drawHeader(engine_, duel_, ui_, ...)` (DuelPanels.hpp:30-66) —
   "P1 \<lp0\>   P2 \<lp1\>   T:P\<turnPlayer+1\>  \<phase name\>", plus a
   center hint chosen from `duel.result / ui_.handoff / st.chainPrompt /
   st.mode` (yellow text).
2. `grid_.drawField(matRect, field_, images_, cardBack_.get())` —
   FieldGrid iterates its 6×9 `grid_` matrix: each `ZoneCell::draw` paints
   the zone-tint rectangle (`COLOR_ZONE_*_BG`), zone label, and card
   (texture via `cache.Get` → `DrawTexturePro`, else colored fallback);
   the cursor cell gets a yellow border, the selected zone a green one.
3. `grid_.drawOpponentHand(...)` — face-down card backs only (never
   revealed); `grid_.drawOwnHand(...)` — the viewer's real cards.
4. `zoneInfoPanelDraw` (ZoneInfoPanel.hpp:16-90) — the right panel shows the
   cursor zone's type/count/ATK-DEF/visibility, "SOURCE SELECTED" when
   `st.selectedZone != nullptr`, the numbered action list (yellow cursor
   line), and `st.lastResult` (green if it contains "OK"/"true"/"Moved",
   red otherwise).
5. `preview_.SetCard(grid_.cursorCard(field_), faceDown)` + `preview_.Draw`
   (CardPreview.hpp:31-118) — portrait image, name, type tag, stat line,
   wrapped description with scroll bar.
6. `DuelPanels::drawFooter` + `DuelPanels::drawOverlays` (DuelPanels.hpp:79-142)
   — pass-device gate (full-screen dim + "PASS THE DEVICE TO PLAYER N"),
   win banner ("PLAYER N WINS! press R for a rematch"), chain banner
   ("Chain open (N links)…"), help modal (H).
7. `images_.PollAndLoad()` (CardImageCache.hpp:81-99) — uploads textures
   finished by the worker thread this frame.

---

## 11. The hotseat loop (full turn sequence, values at every step)

1. Player 1 wins the toss = starting player; `turnCount = 1`.
2. P1's first turn: handoff gate → SPACE → draw **0 cards** (`firstTurn`
   flag), Standby → Main1 → summon/set/activate (`normalSummonUsed_` true
   after one summon) → B (Battle, but **P1 cannot attack on turn 1** —
   `canAttack`/`guardAttack` refuse) → E → discard down to 6 if needed →
   `endTurn()` swaps sides → `ui_.handoff = true`.
3. P2's turn: SPACE → **draw 1** → full phase cycle: Main1 (summons,
   sets, spells) → B → SPACE attacks (§9) → N (Main2, more summons/sets) →
   E → hand limit → handoff.
4. Every effect/battle resolution re-checks `checkWinConditions()`;
   the duel ends the moment `result != Ongoing`.

**State that flips during every turn, summarized:**
`ui_.handoff` (gate open/closed) → `ui_.mode`
(Navigate→Menu/TributeTarget/EffectTarget/AttackTarget→Navigate) →
`ui_.actions/actionCursor` (rebuilt per menu) → `ui_.lastResult` (every
action) → `duel_.turnPlayer / turnCount / turn.phase` (E) →
`lp[0]/lp[1]` (battle/effects) → zone occupancy/visibility/orientation
(summons, sets, position changes, destruction) → `Card::location` on every
move → per-turn engine sets (`normalSummonUsed_`, `attacked_`,
`flipSummoned_`, `positionChanged_`) cleared by `endTurn()` →
`duel_.result` → `DuelResult::Ongoing` when someone hits 0 LP.

---

## 12. Rematch and quitting the game

**Rematch:** with `duel_.result != Ongoing`, `R` → `DuelScreen::rematch()`
(DuelScreen.hpp:347-357): constructs a fresh `Duel duel_` (8000/8000 LP,
`turnCount=0`, `result=Ongoing`), builds/reshuffles both decks from
`ctx_.selectedDeck`, resets opening hands, and calls `engine_.hardReset()`
(Effects.hpp:78-84) — `resetPerTurnState()`, `battleStep_=Idle`,
`damageStep_=None`, `lastDamageOutcome_=None`, `battleTrace_.clear()` —
and `ui_.hardReset()` clears the entire `DuelUIState`. The flow returns to
step 7 of §7 (handoff gate → P1's first draw-less turn).

**Quitting — exactly two exits:**

1. *MainMenu* → highlight **Quit** (last item) → ENTER/SPACE →
   `ScreenEvent::quit()` (MainMenuScreen.hpp:28) → `App::Run` line 99
   `break`s → destructor chain (§4).
2. *Any screen* → the window's close button → `WindowShouldClose()` turns
   true at line 94 → loop exits → same destructor chain.

There is deliberately **no ESC-quit and no exit from the Duel screen** —
duels only ever end back at the main menu or in a rematch.

---

## 13. The other screens (execution notes)

### 13.1 SettingsScreen (screens/SettingsScreen.hpp)

`Update()`: reads `AppConfig` fields via `cfg.get…(Key::…)` and writes them
back with `cfg.set…` on the toggle keys — settings persistence happens
*per keypress*. **Values changed:** the live `AppConfig` singleton. `ESC` →
`ScreenEvent::replace(AppScreen::MainMenu)`.

### 13.2 TestingScreen (screens/TestingScreen.hpp) — cards.json inspection

- Reads `AppContext::cardsDB` (filled by `App::LoadCards`, §5) into
  `std::vector<Card> cards_` on entry.
- `KeyboardNav cardsNav_` (KeyboardNav.hpp, clamping) — UP/DOWN move
  `cursor`; PGUP/PGDN jump 10. The cursor indexes `cards_`.
- Enter cycles the **sort mode** over `cards_` (name/level/ATK/DEF/id …);
  the search field is the shared `TextInput` widget
  (TextInput.hpp:25-44: right-click toggles `typing_`,
  `GetCharPressed()` loop appends printable chars, Backspace pops,
  ESC stops typing; `changed_` true on any edit → list refilters).
- ESC → back to the main menu.

### 13.3 DeckEditorScreen — construction loop (screens/DeckEditorScreen.hpp)

On entry `pool_ = all cards` from the DB; `deck_ = ctx_.selectedDeck`.
`Update()` (lines 80-182):

1. `ESC` (when the search field isn't typing) → `replace(MainMenu)`.
2. `searchInput_.Update()` → if `changed_`, `filterPool(pool_, typeFilter_,
   query)` (DeckFilters.hpp:61-79: case-insensitive substring + type filter)
   rebuilds the visible pool pointer list.
3. T cycles `typeFilter_` (All/Monster/Spell/Trap), S-key cycles
   `sortMode_` → `sortPool()` (DeckFilters.hpp:45-58).
4. **Pool focus** (TAB/right switches focus): `poolNav_.handleClampKeys()`
   moves the cursor; **ENTER adds** the highlighted card to `deck_` — but
   only if `deck_.size() < 60` and `countCopies(deck_, id) < 3`
   (DeckLimits, DeckFilters.hpp:18-22); otherwise `statusMsg_` explains.
5. **Deck focus:** arrows move `deckNav_` (grid view steps by row),
   DELETE/Backspace/D **erase** `deck_[deckNav_.cursor]`, G toggles
   list/grid view.
6. **S → `SaveDeck("default")`** → `DeckFile::Write(path, deck_)`
   (DeckFile.hpp:18-23: one numeric `cardId` per line, `#` comments).
   **L → `LoadDeck("default")`** → `DeckFile::Read` (DeckFile.hpp:27-44:
   `std::stoul` per line, `db.GetCardById` resolves, unknown ids skipped)
   → `deck_` replaced.
7. **D** → if `deck_.size() >= 40`: `ctx_.selectedDeck = deck_` (**values
   change:** the 40-60 card list now travels through AppContext) →
   `replace(AppScreen::Duel)`.

### 13.4 DeckEditorScreen::Draw()

Draws both panes each frame: `CardGrid` (list or 4-column grid view) for the
filtered pool and for the deck, the `CardPreview` panel for
`poolNav_/deckNav_` cursor card, deck-stat lines (counts by type), the
`searchInput_.Draw(...)`, and `statusMsg_`. Same `CardImageCache::Get`
fallback chain as the duel.

---

## 14. State quick-reference — who owns what

| Owner | Object | Key mutable state |
|---|---|---|
| `main()` | `AppConfig cfg` | window size, card/image URLs, DB path |
| `main()` | `AppContext ctx` | `CardDatabase cardsDB`, `CardImageCache*`, `selectedDeck` (40-60 `Card` copies) |
| `App` | `ScreenManager` | one live `IScreen` at a time |
| `DuelScreen` | `Duel duel_` | `lp[2]` (8000→0), `turnPlayer` (0/1), `turnCount`, `turn.phase` (Draw/Standby/Main1/Battle/Main2/End), `result` (Ongoing/Player0Win/Player1Win/Draw), `chain.links/step`, `firstTurn` |
| `DuelScreen` | `Engine engine_` | `normalSummonUsed_`, `attacked_`, `flipSummoned_`, `positionChanged_`, `pending_`, `battleStep_`, `damageStep_`, `lastDamageOutcome_`, `battleTrace_`, `resolver_` |
| `DuelScreen` | `zone::Field field_` | all zones for both players (§7.1); every `put/remove/flip/changeOrientation/changeVisibility` |
| `DuelScreen` | `DuelUIState ui_` | `mode`, `cursorRow/Col`, `handCursor`, `selectedZone`, `attacker/pendingTarget/pendingDirect`, `pendingCard/pendingFx/pendingOwner`, `tributePicks/tributeCount`, `fusionPending/ritualPending`, `chainPrompt`, `handoff`, `actions/actionCursor`, `lastResult`, `helpOpen`, `activated` |
| `DuelScreen` | helpers | `grid_` (6×9 zone matrix + cursor + viewer), `actions_`, `fx_`, `panels_`, `preview_`, `images_` (shared), `cardBack_` |
| `CardImageCache` | worker thread | `jobQueue_`, `queued_`, `completed_`, `textures_` (`unordered_map<id, Texture2D>`) |
| `MainMenuScreen` | `KeyboardNav nav_` | `cursor` (0-4) |
| `DeckEditorScreen` | lists | `pool_`, `deck_`, `poolNav_`, `deckNav_`, `focusPool_`, `typeFilter_`, `sortMode_`, `deckGridView_`, `statusMsg_` |
| `TextInput` | widget | `text_`, `typing_`, `changed_` |
| `Card` | per card | `owner`, `controller`, `location`, `setThisTurn`, `placedThisTurn`, `equippedCards`, `counters` (definition fields `name/atk/def/level/effects` stay constant) |
| `Zone`/`ZoneStack` | per zone | `card_` / `cards_`, `ori_` (Vertical/Horizontal), `vis_` (Visible/Limited/Restricted) |

## 15. Glossary of the value-flow rules

- **A card never changes zones by itself.** Every relocation is
  `IZone::remove(c)` → `dest.put(c)` (or `moveTo`), followed by
  `effect_detail::setLoc` updating `Card::location`; failed `put` always
  rolls back (EffectsBuiltIn.hpp:48-63).
- **LP changes** happen only inside the engine (`damage`/`gain`), reached
  from battle math, effect costs, and chain links.
- **Visibility is a zone property**, not a card property: set =
  `Visibility::Limited`, flip = `Vertical+Visible`, decks are `Restricted`,
  hands `Limited`.
- **The chain is the only async structure:** `push` at activation
  (costs paid immediately), resolution strictly reverse-order in
  `resolveChainImpl`, then `chain.clear()`.
- **Every turn boundary** (`endTurn`) is the only place per-turn sets are
  cleared; `hardReset()` (rematch) additionally resets battle bookkeeping.
- **The UI never mutates gameplay state directly** — it calls
  `Engine`/`DuelEffects`/`DuelActions` methods and renders whatever the
  `Duel`/`Field` objects now say.

---

*Manual covers commit `1e9fa0a` on `main` (OpenJoey2). Line numbers refer to
the header-only implementation files as of that commit.*
