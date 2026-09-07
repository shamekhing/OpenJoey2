# Cards layer — API contract (`include/cards/`)

Version 0.3.0 · header-only · C++17 · raylib-free · depends on the core
vocabulary + nlohmann/json. Namespace: `openjoey::cards`.

## 1. Canonical Card surface (read this before touching anything)

`cards/Card.hpp` is the **legacy surface and it is canonical**:

```cpp
struct Card : CardDef {          // CardDef: id, name, description, atk/def/level
    CardState state;             // owner/controller, equip/counters, isToken, …
    bool operator==(const Card&) const;   // identity: id != 0 && id == other.id
    int effectiveAtk() const;    // max(0, atk + state.atkMod)  (same for DEF)
};
```

There is **no** `cardId` (the member is `id`), no `type`/`CardType`, no
`frameType`, no `imageId`, and **no `effects` vector**. Frame/subtype lives in
the `Attribute` list (`cards/CardEnums.hpp` — one enumerator per name):
`isMonster()/isSpell()/isTrap()` and `isExtraDeckMonster()` derive from it, as
does `hasAttribute(a)` / `hasAttributes(list, any)`. Do not reintroduce the
removed members — the engine, UI and tests are written against this surface.

## 2. Parser (`cards/CardParser.hpp` + `utils/JsonUtils.hpp`)

`parseRemoteCardJson(content) -> ParseResult { cards, errors, ok() }`

* input: remote payload `{ "data": [ … ] }`; never aborts on bad entries —
  problems are collected, valid cards kept;
* dedup by `id`, first entry wins; id-less entries dropped; nameless entries
  become `"Card <id>"`;
* frame → attribute mapping (`detail::cardFromRemoteJson`): monster/spell/trap
  family, spell/trap icons (Equip, Continuous, Field, QuickPlay, Counter,
  Ritual), extra-deck mechanics (Fusion, Ritual, Synchro, Xyz). **No**
  `Attribute::Effect` is emitted.

## 3. Database (`cards/CardDatabase.hpp`)

Owns every `Card` in a `vector<Card>` (stable addresses) and hands out
non-owning pointers. Movable, not copyable. API: `LoadFromFile` /
`LoadFromString` / `Clear` · `GetCardById` / `GetCardByName` (nullptr when
missing) · `FindByName` (substring, sorted by id) · `GetCardByAttribute` ·
`GetAllCards()` (read-only). Mutating the vector invalidates the id/name index
and is deliberately impossible.

## 4. Comparators (`cards/CardCompare.hpp`)

`compare::byName / byId / byLevel / byAtk / byDef / byFrame` — strict weak
orderings, name tiebreak; `byFrame` derives Monster < Spell < Trap from the
attribute list (there is no `type` member to sort on).
