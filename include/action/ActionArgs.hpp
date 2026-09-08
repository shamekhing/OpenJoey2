#pragma once
// ── ActionArgs — runtime parameters for one action invocation ────────────────
#include <string>
#include <vector>

#include "action/ActionSpec.hpp"

namespace openjoey::cards {
struct Card;
}

namespace openjoey::engine::action {

struct ActionArgs {
    cards::Card *target = nullptr;         // explicit target card
    cards::Card *source = nullptr;         // the acting card (equips)
    int targetPlayer = -1;                 // explicit player override (>= 0)
    int n = 1;                             // count (draws / damage / counters)
    bool faceDown = false;                 // set / face-down banish variants
    int atk = 0;                           // token stats (Summon_Token)
    int def = 0;                           // token stats (Summon_Token)
    ActionSpec spec;                       // Activate* actions: the spec to run
    std::vector<cards::Card *> materials;  // fusion / ritual materials
};

}  // namespace openjoey::engine::action
