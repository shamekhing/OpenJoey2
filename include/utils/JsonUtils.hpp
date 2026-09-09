#pragma once
// ── Internal JSON mapping helpers for CardParser. ────────────────────────────
// NOT part of the public API: include "cards/CardParser.hpp" instead.
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace openjoey::cards::detail {

// Remote providers occasionally encode stats as "?" or strings; anything
// unparseable maps to 0.
inline int parseStatField(const nlohmann::json &j, const char *key) {
    if (!j.contains(key)) return 0;
    const auto &v = j.at(key);
    if (v.is_null()) return 0;
    if (v.is_number_integer()) return static_cast<int>(v.get<int>());
    if (v.is_number_unsigned()) return static_cast<int>(v.get<unsigned>());
    if (v.is_number_float()) return static_cast<int>(v.get<double>());
    if (v.is_string()) {
        const std::string s = v.get<std::string>();
        if (s == "?" || s.empty()) return 0;
        try {
            return std::stoi(s);
        } catch (...) { return 0; }
    }
    return 0;
}

inline int optIntMember(const nlohmann::json &j, const char *key, int def = 0) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null() || !it->is_number()) return def;
    if (it->is_number_unsigned()) return static_cast<int>(it->get<uint64_t>());
    if (it->is_number_integer()) return static_cast<int>(it->get<int64_t>());
    return static_cast<int>(it->get<double>());
}

inline std::string optStringMember(const nlohmann::json &j, const char *key, const std::string &def = {}) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null() || !it->is_string()) return def;
    return it->get<std::string>();
}

inline uint32_t optCardId(const nlohmann::json &j) {
    auto it = j.find("id");
    if (it == j.end() || it->is_null()) return 0;
    if (it->is_number_unsigned()) return static_cast<uint32_t>(it->get<uint64_t>());
    if (it->is_number_integer()) {
        const auto v = it->get<int64_t>();
        if (v <= 0) return 0;
        return static_cast<uint32_t>(v);
    }
    return 0;
}

// Remote card-data entry → Card (legacy surface: id + attributes). Only
// well-formed numeric/string members are read; nothing here throws for
// missing fields.
inline Card cardFromRemoteJson(const nlohmann::json &j) {
    Card c;
    c.id = optCardId(j);
    c.name = optStringMember(j, "name");
    c.description = optStringMember(j, "desc");

    // Frame/type mapping onto the legacy attribute list.
    const std::string frame = optStringMember(j, "frameType");
    auto add = [&](Attribute a) { c.attributes.push_back(a); };
    if (frame == "spell" || frame == "skill") {
        add(Attribute::Spell);
        if (frame != "skill") {
            const std::string icon = optStringMember(j, "race");  // spell icon
            if (icon == "Equip") add(Attribute::Equip);
            else if (icon == "Continuous") add(Attribute::Continuous);
            else if (icon == "Field") add(Attribute::Field);
            else if (icon == "Quick-Play" || icon == "QuickPlay") add(Attribute::QuickPlay);
            else if (icon == "Ritual") add(Attribute::Ritual);
            else add(Attribute::Normal);
        }
    } else if (frame == "trap") {
        add(Attribute::Trap);
        const std::string icon = optStringMember(j, "race");  // trap icon
        if (icon == "Continuous") add(Attribute::Continuous);
        else if (icon == "Counter") add(Attribute::Counter);
        else add(Attribute::Normal);
    } else {
        add(Attribute::Monster);
        if (frame == "fusion") add(Attribute::Fusion);
        else if (frame == "ritual") add(Attribute::Ritual);
        else if (frame == "synchro") add(Attribute::Synchro);
        else if (frame == "xyz") add(Attribute::Xyz);
        else add(Attribute::Normal);
    }

    c.atk = parseStatField(j, "atk");
    c.def = parseStatField(j, "def");
    c.level = optIntMember(j, "level", 0);
    const int rank = optIntMember(j, "rank", 0);
    if (c.level == 0 && rank > 0) c.level = rank;  // Xyz monsters carry rank, not level

    return c;
}

}  // namespace openjoey::cards::detail
