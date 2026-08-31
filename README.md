# OpenJoey2 — umbrella repo

This repository is now a thin umbrella. All code and content live in six
independent repositories that sit side-by-side with it on disk:

| Repo | Owns |
|---|---|
| `openjoey-core` | Shared contracts (`EffectID`, `AppConfig`, `Settings`) + nlohmann/json |
| `openjoey-uikit` | Shared raylib rendering toolkit (StyleSheet, DrawUtils, widgets) |
| `openjoey-cards` | Card domain: definitions, parsing, database, card widgets |
| `openjoey-gameplay` | Duel rules: zone, field, duel logic + per-repo test suite |
| `openjoey-app` | App shell, screens, duel presentation, entry point, superbuild |
| `openjoey-content` | Data, asset-generation + fetch scripts, rulebook |

Dependency chain: `core -> uikit -> cards -> gameplay -> app` (content is data-only).

## Building

Clone the repos side-by-side, then build from `openjoey-app` (it is the
superbuild and pulls in the siblings automatically):

```sh
cmake -S openjoey-app -B openjoey-app/build/release -DCMAKE_BUILD_TYPE=Release
cmake --build openjoey-app/build/release -j"$(nproc)"
ctest --test-dir openjoey-app/build/release --output-on-failure
./openjoey-app/build/release/OpenJoey2
```

```sh
cmake -S openjoey-app -B openjoey-app/build/debug -DCMAKE_BUILD_TYPE=Debug
cmake --build openjoey-app/build/debug -j"$(nproc)"
ctest --test-dir openjoey-app/build/debug --output-on-failure
./openjoey-app/build/debug/OpenJoey2
```

Every repo also configures and tests standalone (each bootstraps its own
dependencies — see its README).

Content (`cards.json`, card images) is not stored in git: fetch it with
`openjoey-content/scripts/fetch_cards.py` or download the `content-latest`
release from the `openjoey-content` repository.

## Testing

Testing is per repo — `openjoey-gameplay` owns the engine test suite
(`ctest`), each other repo owns its own checks. This umbrella repo has none.

Licensed under the GNU GPL-3.0 (see `LICENSE`).
