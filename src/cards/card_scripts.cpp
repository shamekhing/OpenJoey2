// Card -> action link implementation (cards/card_scripts.cpp).
#include "cards/card_scripts.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <map>

namespace openjoey::cards {

namespace {

std::map<uint32_t, ActionSpec> g_scripts;  // overlay: card id -> spec

// Transitional op-name mapping: card_actions.json uses the FINAL vocabulary
// names (Move_ToGY / LP_Change / Chain_Negate / Equip_Attach); the engine enum
// still carries the 47-id names until the vocabulary rename lands.
// TODO(v3-rename): collapse this table to the identity.
const std::map<std::string, ActionId> kOpNames = {
    {"Move_Draw", ActionId::Move_Draw},
    {"Move_MillToGY", ActionId::Move_MillToGY},
    {"Move_DiscardToGY", ActionId::Move_DiscardToGY},
    {"Move_ToGY", ActionId::Move_DestroyToGY},
    {"Move_Banish", ActionId::Move_Banish},
    {"Move_ReturnHand", ActionId::Move_ReturnHand},
    {"Move_ReturnDeck", ActionId::Move_ReturnDeck},
    {"Move_SearchToHand", ActionId::Move_SearchToHand},
    {"Move_Excavate", ActionId::Move_Excavate},
    {"Cost_Tribute", ActionId::Cost_Tribute},
    {"Cost_Discard", ActionId::Cost_Discard},
    {"Cost_PayLP", ActionId::Cost_PayLP},
    {"Cost_BanishCost", ActionId::Cost_BanishCost},
    {"Summon_Normal", ActionId::Summon_Normal},
    {"Summon_Special", ActionId::Summon_Special},
    {"Summon_Fusion", ActionId::Summon_Fusion},
    {"Summon_Ritual", ActionId::Summon_Ritual},
    {"LP_Change", ActionId::LP_Damage},   // sign decides damage vs gain
    {"Chain_Negate", ActionId::NegateActivation},
    {"Equip_Attach", ActionId::Equip_Equip},
    {"Equip_Detach", ActionId::Equip_Unequip},
    {"Counter_Place", ActionId::Counter_Place},
    {"Counter_Remove", ActionId::Counter_Remove},
};

// Scope names use the final vocabulary too (Target vs the current Targeted).
const std::map<std::string, Scope> kScopeNames = {
    {"None", Scope::None},
    {"Target", Scope::Target},
    {"Targeted", Scope::Target},
    {"Activator", Scope::Activator},
    {"Opponent", Scope::Opponent},
    {"PerOppMonster", Scope::PerOppMonster},
    {"OppMonsters", Scope::OppMonsters},
    {"AllMonsters", Scope::AllMonsters},
    {"AllSpellsTraps", Scope::AllSpellsTraps},
    {"OppAttackPos", Scope::OppAttackPos},
};

const std::map<std::string, ActionType> kTimingNames = {
    {"Ignition", ActionType::Ignition}, {"Trigger", ActionType::Trigger},
    {"Quick", ActionType::Quick}, {"Continuous", ActionType::Continuous},
    {"Cost", ActionType::Cost},
};

template <typename Enum>
bool TryEnum(const std::map<std::string, Enum> &table, const std::string &name, Enum &out) {
    auto it = table.find(name);
    if (it == table.end()) return false;
    out = it->second;
    return true;
}

}  // namespace

int LoadCardScripts(const std::string &path) {
    std::ifstream f(path);
    if (!f) return -1;
    nlohmann::json root;
    try {
        f >> root;
    } catch (const nlohmann::json::exception &) {
        return -1;
    }
    if (!root.is_object() || !root.contains("scripts") || !root.at("scripts").is_object()) return -1;

    int loaded = 0;
    for (auto &[key, entry] : root.at("scripts").items()) {
        const uint32_t id = static_cast<uint32_t>(std::stoull(key));
        ActionSpec spec;
        if (entry.contains("id")) {
            std::string name = entry.at("id").get<std::string>();
            TryEnum(kOpNames, name, spec.id);
        }
        std::string timing = entry.value("timing", "Ignition");
        TryEnum(kTimingNames, timing, spec.timing);
        spec.speed = static_cast<uint8_t>(entry.value("speed", 1));
        spec.lpCost = entry.value("lpCost", 0);
        spec.needsTarget = entry.value("needsTarget", false);
        if (entry.contains("actions") && entry.at("actions").is_array()) {
            for (const auto &a : entry.at("actions")) {
                Action action;
                std::string op = a.value("op", "");
                if (!TryEnum(kOpNames, op, action.op)) continue;
                std::string scope = a.value("scope", "None");
                TryEnum(kScopeNames, scope, action.scope);
                action.amount = a.value("amount", 1);
                action.needsTarget = a.value("needsTarget", false);
                spec.actions.push_back(action);
            }
        }
        g_scripts[id] = std::move(spec);
        ++loaded;
    }
    return loaded;
}

const ActionSpec *ScriptFor(uint32_t card_id) {
    auto it = g_scripts.find(card_id);
    return it == g_scripts.end() ? nullptr : &it->second;
}

ActionSpec DeriveScript(const Card &card) {
    ActionSpec s;  // id stays None: derivation never invents ops
    // Layer 1: spell/trap race decides timing + spell speed for every card.
    if (card.hasAttribute(Attribute::Counter)) { s.timing = ActionType::Quick; s.speed = 3; }
    else if (card.hasAttribute(Attribute::QuickPlay)) { s.timing = ActionType::Quick; s.speed = 2; }
    else if (card.hasAttribute(Attribute::Continuous)) { s.timing = ActionType::Continuous; s.speed = 1; }
    else if (card.isTrap()) { s.timing = ActionType::Trigger; s.speed = 2; }
    else if (card.isSpell()) { s.timing = ActionType::Ignition; s.speed = 1; }
    // Flip monsters are trigger candidates (realization is spec-provider work).
    if (card.hasAttribute(Attribute::Flip)) s.timing = ActionType::Trigger;
    return s;
}

ActionSpec ResolveScript(const Card &card) {
    ActionSpec s = DeriveScript(card);
    if (const ActionSpec *overlay = ScriptFor(card.id)) {
        s.id = overlay->id;
        s.timing = overlay->timing;
        s.speed = overlay->speed;
        s.lpCost = overlay->lpCost;
        s.needsTarget = overlay->needsTarget;
        s.actions = overlay->actions;
        s.note = overlay->note;
    }
    return s;
}

bool HasScript(uint32_t card_id) { return ScriptFor(card_id) != nullptr; }

}  // namespace openjoey::cards
