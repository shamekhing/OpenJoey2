# openjoey-foundation — API contract

Version 0.3.0 · header-only · C++17 · raylib-free · PolyForm-Noncommercial-1.0.0

The bottom layer: shared vocabulary + configuration + content data. Depends on
nothing but nlohmann/json (pinned v3.12.0 when fetched). **Must never depend
on raylib, any UI toolkit, or any other openjoey module.**

## 1. Consumers

```
openjoey-foundation <- openjoey-cards <- openjoey-engine <- openjoey-app
                                              └── openjoey::ai (future repo)
```

## 2. Entry point

```cpp
#include <openjoey/foundation.hpp>   // ActionId + ActionSpec + ai + Config
```

## 3. `action/ActionId.hpp` — the action vocabulary

`openjoey::ActionId : uint16_t` — **213 actions** (complete classic-ruleset
extraction), one per line grouped by ruleset section. Player actions and card
effects are the same vocabulary. Appended-only ABI (leading values pinned by
the foundation test).

## 4. `action/ActionSpec.hpp`

* `openjoey::EffectType` — Ignition / Trigger / Quick / Continuous / Cost.
* `openjoey::TargetScope` — None, Targeted, Activator, Opponent, PerOppMonster,
  OppMonsters, AllMonsters, AllSpellsTraps, OppAttackPos.
* `openjoey::ActionSpec` — `{id, timing, speed, amount, lpCost, scope,
  needsTarget, note}`. Brace-init ABI is pinned by the cards test.

## 5. `ai/Ai.hpp` — reserved contracts (future `openjoey::ai` repo)

* **player** (RL): consumes `Engine::observe(viewer)` +
  `Engine::legalActions(player)` + flow applies; deterministic via seeded
  `DuelConfig`.
* **reader**: `infer(const cards::CardDef&) → std::vector<ActionSpec>` — same
  role as the engine's Catalog (which doubles as seed training labels).

## 6. `Config.hpp` — the ONE configuration type

Paths + remote provider endpoints (config-supplied, never compiled in) +
window options (absorbed AppConfig). Layered `Load()`: compiled defaults →
`data/settings.json` → `data/user_settings.json`. `Save()` writes the overlay.
Statics: `exeDir`, `resolveDataFile`, `settingsFile`, `referenceFile`.

## 7. Data & scripts

`data/` — cards.json (release asset), classic_cards.json, decks/, settings.json.
`scripts/` — fetch_cards.py, make_assets.py, make_classic_cards.py.
