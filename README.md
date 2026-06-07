# OpenJoey2

OpenJoey2 is a Yu-Gi-Oh-style duel prototype with a browser canvas UI backed by
a C++ rules core compiled to WebAssembly.

The active app is static and lives in `src/web`, so it can run locally from a
plain HTTP server and deploy directly to GitHub Pages.

## Current Status

```text
Web UI:       static canvas app in src/web
C++ core:     src/cpp, exported through src/cpp/WasmApi.cpp
WASM bundle:  src/web/openjoey_core.js, built as an Emscripten single file
Card DB:      src/web/cards-db.js, compact generated rows
Card images:  loaded from the YGOProDeck image CDN
Native build: root CMake builds a smoke executable for the current core
```

The web layer should stay a UI shell around the C++/WASM API. Put duel rules,
zone rules, phase rules, effect resolution, and win conditions in C++ rather
than JavaScript.

## Repository Layout

```text
.
├── .github/workflows/pages.yml  GitHub Pages deployment workflow
├── CMakeLists.txt               Root native smoke build
├── OpenJoey2.sh                 Root build/run helper
├── README.md
├── data/                        Source card/deck/assets data, ignored locally
└── src/
    ├── cpp/                     C++ rules/core and C ABI
    ├── tools/                   Data-generation tools
    └── web/                     Static browser app published to Pages
```

## GitHub Pages

Pages is deployed by GitHub Actions from:

```text
src/web
```

The workflow is:

```text
.github/workflows/pages.yml
```

In the GitHub repository settings, set Pages source to **GitHub Actions**. The
workflow currently runs on pushes to `main` and `web`, and can also be started
manually with `workflow_dispatch`.

The app is suitable for project Pages paths like:

```text
https://<user>.github.io/<repo>/
```

because `index.html` uses relative asset paths such as `./src/main.js`.

## Run Locally

Use the helper:

```bash
./OpenJoey2.sh -r
```

That serves `src/web` with BusyBox at:

```text
http://localhost:8080
```

You can also use any static server:

```bash
python3 -m http.server 8080 --directory src/web
```

## Build

### Native Smoke Build

The root CMake build compiles `src/cpp/WasmApi.cpp` into `openjoey_core` and
links `src/cpp/NativeSmoke.cpp` into the `OpenJoey2` smoke executable.

```bash
cmake -S . -B /tmp/openjoey2-cmake-check -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/openjoey2-cmake-check --parallel 2
/tmp/openjoey2-cmake-check/OpenJoey2
```

Expected output:

```text
OpenJoey2 native smoke check passed
```

The helper equivalent is:

```bash
./OpenJoey2.sh -n
```

### Web/WASM Bundle

The checked-in web bundle is:

```text
src/web/openjoey_core.js
```

To rebuild it, activate Emscripten and run:

```bash
./OpenJoey2.sh -w
```

The build uses Emscripten `-s SINGLE_FILE=1`, so the WebAssembly binary is
embedded in `openjoey_core.js` as a data URI. There is no separate `.wasm` file
for GitHub Pages to serve.

## Root Build Helper

All script commands are handled by:

```bash
./OpenJoey2.sh
```

Flags:

```text
-s   Configure native CMake debug/release trees from the repo root
-b   Build native debug
-B   Build native release
-x   Execute native debug
-X   Execute native release
-w   Build web/WASM bundle into src/web/openjoey_core.js
-r   Run web app with BusyBox httpd
-W   Build web/WASM, then run web app
-n   Build/run root CMake release smoke target
-c   Clean root build trees
-h   Show help
```

Useful examples:

```bash
./OpenJoey2.sh -r
./OpenJoey2.sh -w
./OpenJoey2.sh -n
OPENJOEY_PORT=9090 ./OpenJoey2.sh -r
```

## Web App Layout

```text
src/web/
├── .nojekyll
├── favicon.png
├── index.html
├── styles.css
├── cards-db.js
├── openjoey_core.js
└── src/
    ├── main.js
    ├── CardDb.js
    ├── DeckBridge.js
    ├── ImageCache.js
    ├── CanvasDeckEditor.js
    ├── core/
    │   ├── App.js
    │   └── Ui.js
    └── screens/
        ├── MainMenuScreen.js
        ├── DeckEditorScreen.js
        ├── DuelScreen.js
        └── TextScreen.js
```

`index.html` loads scripts in this order:

```text
cards-db.js
openjoey_core.js
CardDb.js
ImageCache.js
DeckBridge.js
Ui.js
screen files
App.js
main.js
```

`main.js` creates `window.openJoeyApp` so browser tests and debugging can inspect
runtime status, backend, card count, and active screen state.

## Browser Startup

Startup flow:

```text
1. Read compact card rows from window.OPENJOEY_CARD_ROWS.
2. Create CardDb.
3. Initialize DeckBridge.
4. Prefer C++ WASM if createOpenJoeyCore initializes.
5. Fall back to JS deck/duel helpers if WASM fails.
6. Start the canvas App.
```

Expected runtime status:

```text
Ready (C++ WASM)
```

## Screens

### Main Menu

File:

```text
src/web/src/screens/MainMenuScreen.js
```

Menu entries:

```text
Duel
Deck Editor
Settings
Testing
Quit
```

`Settings` and `Testing` currently route to placeholder text screens.

### Deck Editor

File:

```text
src/web/src/screens/DeckEditorScreen.js
```

Current functionality:

```text
card pool
deck list/grid view
card preview
search
sort modes
type filter
deck stats
localStorage persistence
start duel when deck is legal
```

Controls:

```text
TAB        switch card pool/deck focus
Arrows     navigate
PageUp     fast scroll up
PageDown   fast scroll down
Enter      add selected pool card
Delete     remove selected deck card
Backspace  remove selected deck card
D          remove selected deck card
O          cycle sort mode
T          cycle type filter
G          toggle deck grid/list view
C          clear deck
S          save deck to localStorage
L          load deck from localStorage
F          start duel if deck has at least 40 cards
Escape     return to main menu
```

Deck rules:

```text
minimum deck size: 40
maximum deck size: 60
maximum copies:    3
```

Saved browser deck key:

```text
openjoey2.src2.deck
```

That key name is historical and can be renamed later with a migration fallback.

### Duel Screen

File:

```text
src/web/src/screens/DuelScreen.js
```

Current functionality:

```text
loads player deck from deck editor or fallback card pool
loads opponent from card pool slice
starts duel through bridge
shows both hands
shows monster zones
shows spell/trap zones
shows LP, deck count, hand count, grave count, banished count
shows selected card preview
draws a card
plays own hand card to first open zone
sends selected monster to grave
```

Controls:

```text
TAB       cycle selection mode: hand -> monster -> spell
Arrows    move cursor
Enter     play selected own hand card
D         draw own card
G         send selected own monster to grave
Escape    return to main menu
Click     select hand/field zone
DblClick  play selected own hand card
```

## C++ Core Layout

```text
src/cpp/
├── Type.hpp
├── WasmApi.cpp
├── NativeSmoke.cpp
├── card/
│   ├── Card.hpp
│   ├── CardParser.hpp
│   ├── ICardRepository.hpp
│   └── LocalCardRepository.hpp
├── duel/
│   ├── Chain.hpp
│   ├── DuelCore.hpp
│   ├── PhaseManager.hpp
│   └── ZoneEffectManager.hpp
├── effect/
│   ├── Effect.hpp
│   ├── EffectDescriptor.hpp
│   ├── EffectFactory.hpp
│   ├── EffectRegistry.hpp
│   ├── IDuelContext.hpp
│   ├── access/
│   ├── movement/
│   ├── negate/
│   ├── persistent/
│   └── summon/
├── field/
│   ├── Field.hpp
│   ├── IZone.hpp
│   ├── Zone.hpp
│   ├── ZoneCell.hpp
│   └── ZoneStack.hpp
└── third_party/
    └── nlohmann/json.hpp
```

### Core Ownership Rule

`DuelCore` owns card instances by value in per-player pools.

Zones hold stable `Card *` pointers into those pools. Do not move card ownership
into zones. Zones are containers of non-owning card pointers only.

### Type Naming

Types live under `openjoey::etypes` and use clean enum names:

```text
etypes::card
etypes::location
etypes::position
etypes::zone
etypes::orientation
etypes::visibility
etypes::target
etypes::effect
etypes::spellspeed
etypes::phase
```

Do not reintroduce compatibility aliases such as `enum_card`, `ZoneType`,
`Location`, `Position`, or `Phase`.

### Field Zones

`field/Field.hpp` models:

```text
monsterZones[player][0..4]
spellTrapZones[player][0..4]
fieldZones[player]
extraMonsterZones[0..1]
handZones[player]
deckZones[player]
extraDeckZones[player]
graveyardZones[player]
banishedZones[player]
sideDeckZones[player]
```

`field/Zone.hpp` is the aggregator for:

```text
IZone
ZoneCell / Zone
ZoneStack
Zone_Monster
Zone_SpellTrap
Zone_Field
Zone_ExtraMonster
ZoneStack_Hand
ZoneStack_Deck
ZoneStack_ExtraDeck
ZoneStack_Graveyard
ZoneStack_Banished
ZoneStack_SideDeck
```

## WASM API

File:

```text
src/cpp/WasmApi.cpp
```

The WASM API is a plain C ABI exported by Emscripten. It exposes opaque handles,
numbers, booleans, and card IDs. Avoid exposing C++ classes, STL containers, or
C++ ownership to JavaScript.

Current exported areas:

```text
deck handle lifecycle
deck add/remove/clear
deck count/copy/stat checks
game handle lifecycle
deck staging for each player
duel start
turn/phase/LP/winner access
deck/hand/grave/banished counts
hand card IDs
monster/spell/extra monster zone IDs
draw
play hand card
send monster to grave
advance phase
```

The bridge file is:

```text
src/web/src/DeckBridge.js
```

`DeckBridge.js` should remain thin. It may translate C ABI calls into convenient
screen-facing methods, but it should not become the game engine.

## Data

Source data lives under `data/`, but the GitHub Pages app does not depend on
that local folder at runtime.

The web app loads compact rows from:

```text
src/web/cards-db.js
```

Runtime row format:

```text
[
  id,
  imageId,
  kind,        // 0 monster, 1 spell, 2 trap
  name,
  description,
  atk,
  def,
  level,
  readableType
]
```

Card images are resolved by `src/web/src/CardDb.js` as:

```text
https://images.ygoprodeck.com/images/cards/<imageId>.jpg
```

That keeps the Pages artifact small and avoids publishing local image folders.

## Generating The Compact Card DB

Tool:

```text
src/tools/generate-card-db.js
```

Command:

```bash
node src/tools/generate-card-db.js
```

After regenerating, verify the output path is:

```text
src/web/cards-db.js
```

## Verification

Commands run successfully in the current tree:

```bash
for f in src/web/src/*.js src/web/src/core/*.js src/web/src/screens/*.js; do node --check "$f" || exit 1; done
cmake -S . -B /tmp/openjoey2-cmake-check -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/openjoey2-cmake-check --parallel 2
/tmp/openjoey2-cmake-check/OpenJoey2
node -e 'const create=require("./src/web/openjoey_core.js"); create().then((m)=>{ const d=m._oj_deck_new(); if(!d) throw new Error("deck alloc failed"); if(!m._oj_deck_add(d,89631139,89631139,0,2500,2100,7)) throw new Error("deck add failed"); if(m._oj_deck_count(d)!==1) throw new Error("deck count failed"); m._oj_deck_free(d); console.log("Emscripten module initialized and deck API passed"); })'
```

Chromium headless browser test passed with:

```text
title:          OpenJoey2 Canvas Deck Editor
appReady:       true
status:         Ready (C++ WASM)
backend:        C++ WASM
cardCount:      14341
canvas:         nonblank
browser events: none
```

## GitHub Pages Caveats

File sizes:

```text
src/web/cards-db.js      about 5.6 MB
src/web/openjoey_core.js about 93 KB
```

No checked web file is near GitHub's 100 MB single-file limit.

Path/subpath:

```text
Use relative paths in index.html.
Avoid root-relative paths like /src/main.js.
```

WASM:

```text
The current WASM is embedded in openjoey_core.js.
If a future build emits a separate .wasm file, test Pages MIME/CORS behavior in browsers.
```

## Development Rules

The web UI is responsible for:

```text
canvas drawing
layout
input handling
screen routing
search field positioning
image cache usage
local browser persistence
calling the WASM API
```

The web UI is not responsible for:

```text
duel rules
summon legality
chain rules
effect resolution
zone ownership rules
phase rules
win conditions
```

Keep build entry points at the repo root:

```text
OpenJoey2.sh
CMakeLists.txt
```

Do not add:

```text
src/*.sh
src/CMakeLists.txt
```

## Troubleshooting

### `emcc: command not found`

Install or activate Emscripten, then retry:

```bash
./OpenJoey2.sh -w
```

### Browser Opens But Cards Do Not Show

Check:

```text
src/web/cards-db.js exists
browser console has no path errors
network access to images.ygoprodeck.com is available
the app is served from src/web
```

### Browser Opens But WASM Does Not Load

Rebuild:

```bash
./OpenJoey2.sh -w
```

Then refresh the browser.

If WASM still fails, check the browser console for missing exported functions or
syntax errors in `openjoey_core.js`.

### GitHub Pages Does Not Deploy

Check:

```text
Pages source is set to GitHub Actions
.github/workflows/pages.yml exists
the workflow has pages: write and id-token: write permissions
the uploaded artifact path is src/web
```
