#include "cards/CardParser.hpp"

#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>

namespace openjoey::cards {

bool ParseResult::ok() const { return !cards.empty(); }

ParseResult parseRemoteCardJson(const std::string &content) {
    ParseResult result;

    nlohmann::json root;
    try {
        root = nlohmann::json::parse(content);
    } catch (const nlohmann::json::exception &ex) {
        result.errors.push_back({std::string("JSON parse failed: ") + ex.what()});
        return result;
    }

    if (!root.is_object() || !root.contains("data") || !root.at("data").is_array()) {
        result.errors.push_back({"expected a remote card-data object with a \"data\" array"});
        return result;
    }

    std::unordered_set<uint32_t> seenIds;

    for (const auto &item : root.at("data")) {
        if (!item.is_object()) {
            result.errors.push_back({"skipped non-object entry in data array"});
            continue;
        }
        try {
            Card card = detail::cardFromRemoteJson(item);
            if (card.id == 0) {
                result.errors.push_back({"skipped entry without a valid id"});
                continue;
            }
            if (card.name.empty()) card.name = "Card " + std::to_string(card.id);
            if (!seenIds.insert(card.id).second) {
                result.errors.push_back({"duplicate cardId " + std::to_string(card.id) + " skipped"});
                continue;
            }
            result.cards.push_back(std::move(card));
        } catch (const std::exception &ex) {
            result.errors.push_back({std::string("skipped malformed entry: ") + ex.what()});
        }
    }
    return result;
}

}  // namespace openjoey::cards
