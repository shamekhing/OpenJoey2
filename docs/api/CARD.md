# openjoey-cards — API contract

Version 0.3.0 · header-only · C++17 · raylib-free · PolyForm-Noncommercial-1.0.0

Stable contract between `openjoey-cards` and its consumers (`openjoey-engine`,
`openjoey-app`).

## 1. Module

* Namespace: `openjoey::cards` (parser detail: `cards::detail`, comparators:
  `cards::compare`).
* Target: `openjoey_cards` / alias `openjoey::cards`. Depends on
  `openjoey::foundation` only. **100% raylib-free** (card widgets live in
  openjoey-app, `ui/cards/`).

## 2. Types

* `CardType` — Monster / Spell / Trap.
* `CardDef` — identity: name, cardId, imageId, description, type, frameType,
  atk/def/level, `effects: vector<openjoey::ActionSpec>`.
* `CardState` — duel state: owner/controller, set/placed flags, equips
  (equippedCards + equipTarget + bonusAtk/Def), atkMod/defMod, xyzMaterials,
  counters, isToken. **No location/position** — zones own placement truth.
* `Card : CardDef { CardState state; }` — equality is id-by-identity;
  `effectiveAtk()/effectiveDef()` (base + mods, ≥ 0); presentation helpers.
* `CardParser` — `parseRemoteCardJson(string) → ParseResult` (dedup by id,
  never throws); `detail/cardFromRemoteJson` maps provider fields.
* `CardDatabase` — owns cards (pointer-stable, movable-not-copyable);
  `LoadFromFile/LoadFromString`, `GetCardById/Name`, `FindByName` (sorted).
* `compare::` — byName/byType/byId/byLevel/byAtk/byDef.
