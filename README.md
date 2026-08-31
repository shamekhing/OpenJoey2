# [OpenJoey2](https://github.com/shamekhing/OpenJoey2) — umbrella repo

This repository is now a thin umbrella. All code and content live in six
independent repositories that sit side-by-side with it on disk:

| Repo | Owns | Docs |
|---|---|---|
| [`openjoey-core`](https://github.com/shamekhing/openjoey-core) | Shared contracts (`EffectID`, `AppConfig`, `Settings`) + nlohmann/json | [API](https://github.com/shamekhing/openjoey-core/blob/main/docs/API.md) |
| [`openjoey-uikit`](https://github.com/shamekhing/openjoey-uikit) | Shared raylib rendering toolkit (StyleSheet, DrawUtils, widgets) | [API](https://github.com/shamekhing/openjoey-uikit/blob/main/docs/API.md) |
| [`openjoey-cards`](https://github.com/shamekhing/openjoey-cards) | Card domain: definitions, parsing, database, card widgets | [API](https://github.com/shamekhing/openjoey-cards/blob/main/docs/API.md) |
| [`openjoey-gameplay`](https://github.com/shamekhing/openjoey-gameplay) | Duel rules: zone, field, duel logic + per-repo test suite | [API](https://github.com/shamekhing/openjoey-gameplay/blob/main/docs/API.md) |
| [`openjoey-app`](https://github.com/shamekhing/openjoey-app) | App shell, screens, duel presentation, entry point, superbuild | [API](https://github.com/shamekhing/openjoey-app/blob/main/docs/API.md) |
| [`openjoey-content`](https://github.com/shamekhing/openjoey-content) | Data, asset-generation + fetch scripts, rulebook | [DATA](https://github.com/shamekhing/openjoey-content/blob/main/docs/DATA.md) |

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

## License

Licensed under the [PolyForm Noncommercial License 1.0.0](LICENSE) — free for
personal, educational, research, and other noncommercial use. Any commercial
use requires a paid license from the copyright holder (see `LICENSE`; contact:
shamekhing@gmail.com).
