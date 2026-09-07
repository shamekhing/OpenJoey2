# OpenJoey2

A classic-ruleset Yu-Gi-Oh! engine with a raylib hotseat UI, playable natively
or in the browser. Single monorepo, C++17, header-first.

## Layout (namespace = path)

Every include path mirrors its C++ namespace — one include root (`include/`):

| Path | Namespace | Owns |
|---|---|---|
| `include/Config.hpp` | `openjoey` | layered settings (window, paths, duel-format switches) |
| `include/action/` | `openjoey` | the action vocabulary: `ActionId` (213 actions), `ActionSpec`, `ActionArgs` |
| `include/ai/` | `openjoey::ai` | **reserved** contracts for the RL opponent + card reader (satisfied by `Engine::observe` / `Engine::legalActions`) |
| `include/cards/` | `openjoey::cards` | `Card` (identity + duel state), parsing, `CardDatabase`, comparators — raylib-free |
| `include/engine/action/` | `openjoey::engine::action` | the rules: battle, chains, summons, moves, positions — classic ruleset |
| `include/engine/duel/` | `openjoey::engine` | `Duel` state, `Chain`, the `Engine` facade, undo snapshots |
| `include/engine/{config,field,protocol}/` | `openjoey::engine` | `DuelConfig` (format switches), the mat (zones), protocol walks |
| `include/ui/` | `openjoey::ui` | raylib app: screens, duel/deck components, widgets, image cache |
| `include/utils/` | internal | `JsonUtils` (parser helpers) |
| `src/main.cpp` | `openjoey::app` | entry point |

Dependency chain: `ui → engine → cards → action` (raylib only in `ui`).

## Classic ruleset

The engine implements the classic ruleset: Normal/Tribute/Flip/Special/Fusion
and Ritual summons, ATK/DEF battle math with replays, chains with Spell Speeds,
p.31 Set-turn trap timing, position rules (p.36), hand limit (p.41), and the
p.45 response window as a runtime switch. Post-classic mechanics
(Synchro/Xyz/Pendulum/Link) are explicit classic-format gates.

Card effects resolve from the **name-keyed classic catalog**
(`engine/action/Catalog.hpp`) at activation time — `Card` carries identity and
duel state, not effect text (see `docs/api/CARD.md`).

## Building

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(nproc)"
ctest --test-dir build --output-on-failure      # core + cards + engine suites
./build/OpenJoey2
```

Debug: same with `-DCMAKE_BUILD_TYPE=Debug`. Sanitizers:
`-DOPENJOEY_SANITIZE=address,undefined`.

### Web (Emscripten)

```sh
emcmake cmake -S . -B build-web -DCMAKE_BUILD_TYPE=Release
cmake --build build-web -j"$(nproc)"
python3 -m http.server -d build-web/web     # → http://localhost:8000
```

## Play in the browser

Every push to `main` builds the web target and deploys it:
**https://shamekhing.github.io/OpenJoey2/** (`.github/workflows/pages.yml`;
requires Pages → Source: “GitHub Actions” in the repo settings).

## Controls

Mouse: **click** a card/zone = action menu · click targets · hover inspects ·
**right-click** = cancel.
Keyboard: arrows/WASD move · ENTER use · SPACE attack · B/N Battle/Main2 ·
C position · F flip/tributes · E end turn · R resolve/rematch · **Z undo** ·
**L duel log** · H help.

## Data & scripts

`data/` ships the playable content (`cards.json`, classic set, decks, card
back). `scripts/` has the content pipeline (`fetch_cards.py`,
`make_assets.py`); the bulk card-image cache (`data/images/`) is
runtime-fetched and **deliberately not in git**.

## Docs

`docs/api/{CORE,CARD,ENGINE}.md` — per-layer contracts · `docs/ACTIONS.md` —
the full per-action table (machine-checked by the iterate-all-ids test) ·
`Manual.md` — runtime execution trace.
