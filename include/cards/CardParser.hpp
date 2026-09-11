#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "cards/JsonUtils.hpp"
#include "Card.hpp"

namespace openjoey::cards {
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────────── PARSER ───────────────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────

// A single non-fatal problem found while parsing. Parsing never aborts on a
// bad entry: problems are collected, valid cards are kept.
struct ParseError {
    std::string message;
};

// ParseResult: holds parsed cards and any diagnostics.
// Method declaration only — definition follows below.
//
// Definition order:
//   parseRemoteCardJson (calls detail::cardFromRemoteJson)
//   ParseResult::ok (no deps)
struct ParseResult {
    std::vector<Card> cards;         // valid cards, input order, deduped by id
    std::vector<ParseError> errors;  // per-entry diagnostics
    bool ok() const;
};

bool ok() const;

// parseRemoteCardJson — calls detail::cardFromRemoteJson; defined after it.
//
// Remote card-data API (`{ "data": [ { ... }, ... ] }`).
// Parses a payload already held in memory. Guarantees:
//   * cards are de-duplicated by cardId — first entry wins, input order kept
//   * entries with an unusable id are skipped and reported in `errors`
//   * nameless cards get "Card <id>"
//   * imageId == cardId for every parsed card
ParseResult parseRemoteCardJson(const std::string &content);
}  // namespace openjoey::cards
