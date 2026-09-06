# [OpenJoey2](https://github.com/shamekhing/OpenJoey2) — umbrella repo

This repository is a thin umbrella: CI, shared clang-format, and docs. All code
and content live in **four** independent repos that sit side-by-side on disk.

## The four repos

| Repo | Namespace | Owns |
|---|---|---|
| [`openjoey-foundation`](https://github.com/shamekhing/openjoey-foundation) | `openjoey` | action vocabulary (`ActionId` — 213 actions, `ActionSpec`), Config, content data + fetch pipeline; **`openjoey::ai` reserved** (RL opponent + card reader — see `ai/Ai.hpp` contracts) |
| [`openjoey-cards`](https://github.com/shamekhing/openjoey-cards) | `openjoey::cards` | Card (identity `CardDef` + duel `CardState`), parsing, database, comparators — raylib-free |
| [`openjoey-engine`](https://github.com/shamekhing/openjoey-engine) | `openjoey::engine` | zones, field, **`action/` (one HPP per action ×213 + resolver + catalog)**, duel flow, `config/DuelConfig`, `protocol/` walks — classic ruleset |
| [`openjoey-app`](https://github.com/shamekhing/openjoey-app) | `openjoey::ui` + `openjoey::app` | all UI: widgets (absorbed uikit), cards widgets, duel/deck components, screens, shell |

Dependency chain: `app → engine → cards → foundation` (raylib only in app).

## Classic ruleset

The engine implements the classic ruleset: every action from the ruleset
extraction is realized (`openjoey-engine/docs/ACTIONS.md` — full per-action
table, machine-checked by the iterate-all-ids test). Post-classic mechanics
(Synchro/Xyz/Pendulum/Link) are explicit classic-format gates.

## Building

```sh
cmake -S openjoey-app -B openjoey-app/build/release -DCMAKE_BUILD_TYPE=Release
cmake --build openjoey-app/build/release -j"$(nproc)"
ctest --test-dir openjoey-app/build/release --output-on-failure
./openjoey-app/build/release/OpenJoey2
```

Debug: same with `-DCMAKE_BUILD_TYPE=Debug` and `build/debug`.
Headless chain (foundation → cards → engine) builds and tests raylib-free on
all platforms (see `.github/workflows/ci.yml`).
