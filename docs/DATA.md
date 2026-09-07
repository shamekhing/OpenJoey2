# Data contract

## cards.json

Remote-provider payload: `{ "data": [ { … } ] }`. Parsed by
`cards/CardParser.hpp` (`parseRemoteCardJson` → `ParseResult`):

* `id` (number) — **the** identity; duplicates: first wins, id-less dropped;
* `name`, `desc`, `atk`/`def` ("?"-tolerant → 0), `level` (rank fallback);
* `frameType` → attribute mapping (monster/spell/trap family + icons +
  extra-deck mechanics) — see `docs/api/CARD.md`.

Cards referenced by the UI/engine use `Card::id` (no separate image id — the
image filename is `<id>.jpg`).

## settings.json / user_settings.json

Layered into `openjoey::Config` (see `docs/api/CORE.md`): `window`, `paths`,
`url` (content endpoints consumed by `scripts/fetch_cards.py` and
`CardImageCache`), `app` (`downloadImages`, `chainResponseWindow`,
`autoDiscardEndPhase`).

## decks/*.txt

One card id per line, `#` comments — read by `ui/deck/DeckFile.hpp` against
the database.
