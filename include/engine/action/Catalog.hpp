#pragma once
// ── Classic card → action catalog (THE one wiring table) ─────────────────────
// Maps classic-era cards to their wired ActionSpec so a real cards.json load
// plays without hand-editing card data. This table MERGED the old
// duel/ClassicCatalog.hpp and field/ClassicEffects.hpp (they used to
// disagree). Speeds follow the rulebook: Normal/Effect monsters + Normal
// Spells are Spell Speed 1, Quick-Play + Traps are 2, Counter Traps are 3.
#include <cctype>
#include <string>
#include <unordered_map>
#include <vector>

#include "action/ActionSpec.hpp"

namespace openjoey::engine::action {
using cards::Card;

inline std::string lowerName(const std::string &s) {
    std::string r;
    for (char c : s) r += static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    return r;
}

inline const std::unordered_map<std::string, std::vector<ActionSpec>> &classicActions() {
    using ET = EffectType;
    using TS = TargetScope;
    static const std::unordered_map<std::string, std::vector<ActionSpec>> m = {
        {"pot of greed", {{ActionId::Move_Draw, ET::Ignition, 1, 2, 0, TS::None, false, "draw 2"}}},
        {"graceful charity", {{ActionId::Move_Draw, ET::Ignition, 1, 3, 0, TS::None, false, "draw 3"}, {ActionId::Cost_Discard, ET::Cost, 1, 2, 0, TS::Activator, false, "discard 2"}}},
        {"jar of greed", {{ActionId::Move_Draw, ET::Quick, 2, 1, 0, TS::None, false, "draw 1"}}},
        {"raigeki", {{ActionId::Move_DestroyToGY, ET::Ignition, 1, 1, 0, TS::OppMonsters, false, "all opponent monsters"}}},
        {"dark hole", {{ActionId::Move_DestroyToGY, ET::Ignition, 1, 1, 0, TS::AllMonsters, false, "all monsters"}}},
        {"fissure", {{ActionId::Move_DestroyToGY, ET::Ignition, 1, 1, 0, TS::Targeted, true, "1 opponent monster (lowest ATK)"}}},
        {"mystical space typhoon", {{ActionId::Move_DestroyToGY, ET::Ignition, 1, 1, 0, TS::Targeted, true, "1 spell/trap"}}},
        {"heavy storm", {{ActionId::Move_DestroyToGY, ET::Ignition, 1, 1, 0, TS::AllSpellsTraps, false, "all spells/traps"}}},
        {"nobleman of crossout", {{ActionId::Move_Banish, ET::Ignition, 1, 1, 0, TS::Targeted, true, "1 face-down monster"}}},
        {"dian keto the cure master", {{ActionId::LP_Gain, ET::Ignition, 1, 1000, 0, TS::Activator, false, "gain 1000 LP"}}},
        {"ookazi", {{ActionId::LP_Damage, ET::Ignition, 1, 800, 0, TS::Opponent, false, "800 to opponent"}}},
        {"just desserts", {{ActionId::LP_Damage, ET::Quick, 2, 500, 0, TS::PerOppMonster, false, "500 per opponent monster"}}},
        {"solemn judgment", {{ActionId::NegateActivation, ET::Quick, 3, 0, 2000, TS::None, false, "negate an activation; pay 2000 LP"}}},
        {"delinquent duo", {{ActionId::Cost_PayLP, ET::Cost, 1, 0, 1000, TS::Activator, false, "pay 1000 LP"}, {ActionId::Cost_Discard, ET::Ignition, 1, 1, 0, TS::Opponent, false, "discard 1 from opponent's hand"}}},
        {"monster reborn", {{ActionId::Summon_Special, ET::Ignition, 1, 1, 0, TS::Targeted, true, "special summon 1 monster from any graveyard"}}},
        {"man-eater bug", {{ActionId::LP_Damage, ET::Trigger, 1, 500, 0, TS::Opponent, false, "FLIP: 500 damage"}}},
        {"hane-hane", {{ActionId::Move_ReturnHand, ET::Trigger, 1, 1, 0, TS::Targeted, true, "FLIP: return 1 monster to hand"}}},
        {"giant trunade", {{ActionId::Move_ReturnHand, ET::Ignition, 1, 1, 0, TS::AllSpellsTraps, false, "all spells/traps to owners' hands"}}},
        {"premature burial", {{ActionId::Summon_Special, ET::Quick, 2, 1, 800, TS::Targeted, true, "special summon 1 monster from your GY"}}},
        {"polymerization", {{ActionId::Summon_Fusion, ET::Ignition, 1, 1, 0, TS::None, false, "fusion summon"}}},
        {"black illusion ritual", {{ActionId::Summon_Ritual, ET::Ignition, 1, 1, 0, TS::None, false, "ritual summon"}}},
        {"cyber-stein", {{ActionId::Cost_PayLP, ET::Cost, 1, 0, 5000, TS::Activator, false, "pay 5000 LP"}, {ActionId::Summon_Special, ET::Ignition, 1, 1, 0, TS::Targeted, false, "1 Fusion from your Extra Deck"}}},
        {"trap hole", {{ActionId::Move_DestroyToGY, ET::Trigger, 2, 1, 0, TS::Targeted, true, "1 summoned monster with ATK >= 1000"}}},
        {"mirror force", {{ActionId::Move_DestroyToGY, ET::Trigger, 2, 1, 0, TS::OppAttackPos, false, "destroy all attacking (ATK-pos) opponent monsters"}}},
    };
    return m;
}

// nullptr if the card has no wired classic action (first spec of the card).
inline const ActionSpec *findClassicEffect(const std::string &name) {
    const auto &m = classicActions();
    auto it = m.find(lowerName(name));
    return (it == m.end() || it->second.empty()) ? nullptr : &it->second.front();
}

// Vector form: every wired spec for the card (test + DuelSetup compatibility).
inline std::vector<ActionSpec> classicEffectsFor(const std::string &name) {
    const auto &m = classicActions();
    auto it = m.find(lowerName(name));
    return it == m.end() ? std::vector<ActionSpec>{} : it->second;
}

}  // namespace openjoey::engine::action
