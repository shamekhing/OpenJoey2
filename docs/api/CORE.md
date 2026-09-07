# Core layer — API contract (`include/Config.hpp`, `include/action/`, `include/ai/`)

Version 0.3.0 · header-only · C++17 · raylib-free · depends on nlohmann/json
(pinned v3.12.0 when fetched) and nothing else.

The root vocabulary every other layer speaks. Namespace: `openjoey`.
Consumers: `cards` ← `engine` ← `ui`.

## 1. `action/ActionId.hpp` — the action vocabulary

`enum class ActionId : uint16_t` — every player action **and** card effect is
one id (213 entries, grouped by ruleset section). The numbering is part of the
contract: `None == 0`, `Move_Draw == 5`, `Summon_Normal == 13` (pinned by
`tests/core.cpp`).

## 2. `action/ActionSpec.hpp` — one encoding for cards, chains and menus

```cpp
struct ActionSpec {
    ActionId    id;             // what happens
    EffectType  timing;         // Ignition / Trigger / Quick
    uint8_t     speed;          // Spell Speed 1 / 2 / 3
    int         amount;         // draws / damage / LP / cards moved
    int         lpCost;         // non-refundable activation cost
    TargetScope scope;          // Opponent / AllMonsters / PerOppMonster / …
    bool        needsTarget;    // activation must ask for a target card
    const char *note;           // menu label / debug hint
};
```

`ActionArgs` (`action/ActionArgs.hpp`) is the runtime parameter block:
`target`, `source` (set-turn Trap checks), `targetPlayer`, `n`, `faceDown`,
`materials`.

## 3. `Config.hpp` — layered settings

Load order: compiled defaults → `data/settings.json` (shipped reference) →
`data/user_settings.json` (app-written). Groups: `window` (size/fps/fullscreen),
`paths`, `url` (content endpoints), `app` (`downloadImages` plus the two duel
format switches):

| Field | Default | Meaning |
|---|---|---|
| `chainResponseWindow` | `false` | p.45: chains resolve only after both players pass |
| `autoDiscardEndPhase` | `true` | p.41: `EndTurn` auto-discards down to 6 |

`Save()` writes `user_settings.json`; `Load(argv0)` resolves `data/` beside the
executable (web build: the Emscripten VFS root).

## 4. `ai/Ai.hpp` — reserved contracts

`openjoey::ai` is intentionally empty today; two agents are planned:

* **player** (RL): consumes `Engine::observe(viewer)` (read-only `StateView`)
  and `Engine::legalActions(player)` (the action space, `vector<ActionSpec>`),
  applies actions through the same facade; deterministic episodes come from
  `DuelConfig`'s injectable RNG seed.
* **reader**: `infer(const cards::CardDef&) -> vector<ActionSpec>` — the same
  role `engine/action/Catalog.hpp` plays as a pure data table.
