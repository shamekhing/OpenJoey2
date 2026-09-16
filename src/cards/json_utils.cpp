#include "cards/json_utils.hpp"

#include <cstdint>
#include <string>

#include "cards/card.hpp"

namespace openjoey::cards::detail {

int parseStatField(const nlohmann::json &j, const char *key) {
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

int optIntMember(const nlohmann::json &j, const char *key, int def) {
    auto it = j.find(key);
    if (it == j.end() || it->is_null() || !it->is_number()) return def;
    if (it->is_number_unsigned()) return static_cast<int>(it->get<uint64_t>());
    if (it->is_number_integer()) return static_cast<int>(it->get<int64_t>());
    return static_cast<int>(it->get<double>());
}

std::string optStringMember(const nlohmann::json &j, const char *key, const std::string &def) {
    auto it = j.find(key);
    return (it == j.end() || it->is_null() || !it->is_string()) ? def : it->get<std::string>();
}

uint32_t optCardId(const nlohmann::json &j) {
    auto it = j.find("id");
    if (it == j.end() || it->is_null()) return 0;
    if (it->is_number_unsigned()) return static_cast<uint32_t>(it->get<uint64_t>());
    if (it->is_number_integer()) {
        const auto v = it->get<int64_t>();
        return v > 0 ? static_cast<uint32_t>(v) : 0;
    }
    return 0;
}

Card cardFromRemoteJson(const nlohmann::json &j) {
    Card c;
    c.id = optCardId(j);
    c.name = optStringMember(j, "name");
    c.description = optStringMember(j, "desc");

    auto add = [&](Attribute a) { c.attributes.push_back(a); };

    add(cards::attribute_from_string(optStringMember(j, "type")));
    add(cards::attribute_from_string(optStringMember(j, "frameType")));
    add(cards::attribute_from_string(optStringMember(j, "race")));
    add(cards::attribute_from_string(optStringMember(j, "attribute")));
    add(cards::attribute_from_string(optStringMember(j["banlist_info"], "ban_tcg")));

    c.atk = parseStatField(j, "atk");
    c.def = parseStatField(j, "def");
    c.level = optIntMember(j, "level", 0);
    {
        const int rank = optIntMember(j, "rank", 0);
        if (c.level == 0 && rank > 0) c.level = rank;
    }
    c.scale = optIntMember(j, "scale", 0);
    c.linkRating = optIntMember(j, "linkval", 0);

    // Pendulum cards carry two rules texts; keep both, pendulum text first.
    const std::string pend = optStringMember(j, "pend_desc");
    const std::string monster = optStringMember(j, "monster_desc");
    if (!pend.empty() || !monster.empty()) {
        c.description = pend;
        if (!pend.empty() && !monster.empty()) c.description += "\n";
        c.description += monster;
    }

    // Link markers: "Left,Top" -> LinkMarker* attributes.
    if (j.contains("linkmarkers") && j.at("linkmarkers").is_string()) {
        const std::string markers = j.at("linkmarkers").get<std::string>();
        std::string cur;
        for (const char ch : markers) {
            if (ch == ',') { add(cards::attribute_from_string(cur)); cur.clear(); }
            else cur += ch;
        }
        if (!cur.empty()) add(cards::attribute_from_string(cur));
    }

    // Monster sub-typeline ("Continuous Monster", "Pendulum Effect Monster", ...).
    add(cards::attribute_from_string(optStringMember(j, "typeline")));

    // Ignored by the engine (presentation / collectible): card_sets, card_prices,
    // card_images, ygoprodeck_url, archetype.
    return c;
}

}  // namespace openjoey::cards::detail
