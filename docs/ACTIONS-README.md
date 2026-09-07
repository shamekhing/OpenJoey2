# ActionId — where each action's code lives

The enum here (`action/ActionId.hpp`) declares all 213 actions. Their code:

* **Per-action realization HPP** — `openjoey-engine/include/action/actions/<Name>.hpp`
  (213 files, one per action, `act_<Name>(args)` Engine members)
* **Dispatch** — `openjoey-engine/include/duel/engine/Support.hpp` `perform()`
  (exhaustive switch, iterate-all-ids test proves completeness)
* **Zone-move primitives** — `openjoey-engine/include/action/builtins/` (19)
* **Card → spec wiring** — `openjoey-engine/include/action/Catalog.hpp`
* **Full table** — `openjoey-engine/docs/ACTIONS.md`
