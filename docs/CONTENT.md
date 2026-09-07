# Content

What ships in `data/` (root) versus what is fetched at runtime.

## In git (`data/`)

| Path | Role |
|---|---|
| `cards.json` | remote-format card database (see `docs/DATA.md`) |
| `classic_cards.json` | the curated classic-format set |
| `decks/default.txt` | starter deck (card ids, one per line) |
| `card_back.png` | shared card back (Pillow-generated, see `scripts/`) |
| `settings.json` | shipped-default reference for `Config::Load` |

## Not in git

* `data/images/` — the runtime card-image cache (Konami art; gitignored on
  purpose). Populated by `scripts/fetch_cards.py --images` or downloaded on
  demand by `ui/cards/CardImageCache.hpp` (native build only — the web build
  uses drawn fallback faces).
* Generated assets (`scripts/make_assets.py`) — regeneratable.

## Path contract

`src/main.cpp` passes `argv[0]`; `Config` resolves `data/` beside the
executable. The root CMakeLists symlinks `data/` into the build dir (native)
and preloads it at `/data` (web).
