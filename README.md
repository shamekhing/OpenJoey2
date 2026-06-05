# OpenJoey2

OpenJoey2 is a C++/Raylib Yu-Gi-Oh-style duel prototype with a card database,
deck editor, screen system, field zones, and an effect/duel core. The repo also
contains `src2`, an experimental web/WASM port that keeps game rules in C++ and
renders the UI with a memory-efficient canvas frontend.

## Project Layout

```text
.
├── data/                  Card database, decks, images, UI backgrounds
├── src/                   Current native C++/Raylib application
├── src2/                  Experimental C++/WASM + canvas web app
├── web/                   Earlier static web deck-editor prototype
├── CMakeLists.txt         Native app build entry
└── OpenJoey2.sh           Native app launcher helper
```

## Native App

The current main app lives in `src/`.

```text
src/
├── main.cpp
├── ContentPaths.hpp
├── Type.hpp
├── card/
├── duel/
├── effect/
├── zone/
└── ui/
```

### Startup Flow

`src/main.cpp` creates `openjoey::ui::App` and calls `Run()`.

`App` owns:

- `LocalCardRepository repo_`
- `EffectRegistry effectRegistry_`
- `selectedDeck_`
- `CardImageCache imageCache_`
- `AppContext ctx_`
- `ScreenManager screenManager_`

Startup loads:

```text
data/cards.json
data/effect_registry.json
```

Then it binds effects to cards and opens the main menu.

### Screens

The native app has these screen types:

```text
MainMenuScreen
DeckEditorScreen
DuelScreen
```

Screens implement:

```cpp
ScreenEvent Update(float dt);
void Draw() const;
```

Navigation happens through `ScreenEvent`:

```text
None
Replace(target screen)
Quit
```

### Card System

Cards are plain C++ values:

```cpp
struct Card {
  std::string name;
  uint32_t cardNumber;
  uint32_t imageId;
  std::string description;
  enum_card type;
  int atk;
  int def;
  int level;
  int owner;
  int controller;
  std::vector<Card*> equippedCards;
  std::map<std::string, int> counters;
  std::vector<std::string> effectKeys;
};
```

There are no card subclasses. Card behavior comes from effect keys bound by
`EffectRegistry`.

`LocalCardRepository` loads YGOProDeck-style JSON and provides:

- lookup by ID
- lookup by name
- filtered search
- mutable lookup for effect binding
- add/update/remove
- flush back to JSON

### Deck Editor

`DeckEditorScreen` is keyboard-first.

Controls:

```text
TAB       switch pool/deck focus
Arrows    navigate
PgUp/Dn   fast scroll
ENTER     add selected pool card
DEL/D     remove selected deck card
O         cycle sort mode
T         cycle type filter
G         deck list/grid toggle
C         clear deck
S         save default deck
L         load default deck
F         start duel if deck has 40+ cards
ESC       back/status behavior
```

Deck rules:

```text
minimum deck size: 40
maximum deck size: 60
maximum copies:    3
```

Deck files are stored as card IDs:

```text
data/decks/default.txt
```

### Duel System

The native duel flow uses:

```text
duel/DuelCore.hpp
duel/Chain.hpp
duel/PhaseManager.hpp
duel/ZoneEffectManager.hpp
zone/Field.hpp
zone/Zone.hpp
effect/*
```

`DuelCore` owns the field, card pools, turn state, chain, and effect manager.

Important ownership rule:

```text
pool_[player] owns all card instances by value.
Zones store stable Card* pointers into those pools.
```

That pointer stability is important. Do not move cards into zone-owned storage.

### Effects

Effects are separate objects and descriptors, not card subclasses.

Important files:

```text
effect/Effect.hpp
effect/EffectDescriptor.hpp
effect/EffectFactory.hpp
effect/EffectRegistry.hpp
effect/IDuelContext.hpp
```

Implemented effect areas include:

```text
DrawEffect
DestroyEffect
NegateActivationEffect
ContinuousEffect
NormalSummonEffect
```

## Data

Important data files:

```text
data/cards.json
data/cards_template.json
data/effect_registry.json
data/decks/default.txt
data/images/*.jpg
data/assets/card_back.jpg
data/assets/main_menu_background.png
data/assets/deck_editor_background.png
data/assets/duel_field_background.png
```

`cards.json` is YGOProDeck-format JSON:

```json
{
  "data": [
    {
      "id": 123,
      "name": "...",
      "frameType": "effect",
      "desc": "...",
      "atk": 1000,
      "def": 1000,
      "level": 4
    }
  ]
}
```

## Build Native App

Configure and build:

```bash
cmake -S . -B build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build build/debug
```

Run:

```bash
./build/debug/OpenJoey2
```

Or use:

```bash
./OpenJoey2.sh
```

## src2: Web/WASM Direction

`src2` is the web port direction. The intent is:

```text
C++ / WASM:
  card/deck/duel rules and state

Canvas JavaScript:
  rendering, input, layout, image cache, browser storage
```

This avoids a heavy DOM tree for thousands of cards and keeps the web UI closer
to Raylib's explicit render-loop model.

### src2 Layout

```text
src2/
├── cpp/
│   ├── CardRecord.hpp
│   ├── DeckCore.hpp
│   ├── DeckCore.cpp
│   ├── openjoey/          Copy of the native non-Raylib core from src/
│   ├── WasmApi.cpp
│   └── NativeSmoke.cpp
├── tools/
│   └── generate-card-db.js
├── web/
│   ├── index.html
│   ├── styles.css
│   ├── cards-db.js
│   ├── openjoey_core.js
│   └── src/
└── build-web.sh
```

### src2 C++ API

`src2/cpp/WasmApi.cpp` exposes a plain C ABI for Emscripten.

Deck API examples:

```cpp
DeckCore *oj_deck_new();
bool oj_deck_add(DeckCore *deck, uint32_t id, uint32_t imageId,
                 int kind, int atk, int def, int level);
bool oj_deck_remove_at(DeckCore *deck, int index);
bool oj_deck_can_duel(const DeckCore *deck);
```

Real game API examples:

```cpp
OjGame *oj_game_new();
bool oj_game_add_deck_card(OjGame *game, int player, uint32_t id,
                           uint32_t imageId, int kind, int atk, int def,
                           int level);
bool oj_game_start(OjGame *game);
bool oj_game_draw(OjGame *game, int player);
bool oj_game_play_hand_at(OjGame *game, int player, int handIndex);
uint32_t oj_game_monster_zone_id(const OjGame *game, int player, int zone);
```

### Build src2 Native Smoke Test

```bash
src2/build-native.sh
```

Expected output:

```text
deck smoke ok: 3 cards
```

### Build src2 Web/WASM

Requires Emscripten:

```bash
sudo apt install emscripten
```

Build:

```bash
src2/build-web.sh
```

This writes:

```text
src2/web/openjoey_core.js
```

### Generate Compact Card DB

```bash
node src2/tools/generate-card-db.js
```

This writes:

```text
src2/web/cards-db.js
```

The compact DB strips unused YGOProDeck fields like prices, URLs, and set data.

### Run src2 Web App

Open directly:

```text
src2/web/index.html
```

The app is a canvas UI with:

- main menu
- deck editor
- duel screen backed by copied native `DuelCore`/`Field`
- settings placeholder
- testing placeholder

Current `src2` duel state uses the copied native two-player `DuelCore` and
`Field` through the `oj_game_*` WASM API. The browser screen renders both
players' hands, monster zones, spell/trap zones, deck counts, graveyard counts,
banished counts, LP, phase, and turn player.

## Development Notes

### Do Not Duplicate Full Card JSON in Runtime UI

The full `data/cards.json` is large. For web, generate compact data and only keep
fields used by the UI:

```text
id
imageId
kind
name
description
atk
def
level
readable type
```

### Use Virtual Lists

The card pool contains thousands of cards. Do not create one DOM node per card.
Render only visible rows/items plus a small buffer.

### Use Image Cache

Do not decode all card images. Use a small LRU cache for visible thumbnails and
preview images.

### Keep C++ ABI Plain

For WASM, expose IDs, integers, booleans, and opaque handles. Avoid exposing C++
classes or STL types directly to JavaScript.

Good:

```cpp
bool oj_deck_add(DeckCore *deck, uint32_t id, int kind);
```

Avoid:

```cpp
std::vector<Card> getDeck();
```

## Current Gap

The native non-Raylib core has been copied into `src2/cpp/openjoey` and is
compiled into the WASM bundle. Remaining parity work is mostly UI/action
coverage: expose more `DuelCore` methods through `WasmApi.cpp`, then map them
to canvas controls that match the Raylib widgets.

The correct direction is not to rewrite the rules in JavaScript. Keep rules in
C++ and expose a small WASM API to the canvas UI.
