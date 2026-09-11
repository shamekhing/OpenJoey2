#pragma once
// ── Internal JSON mapping helpers for CardParser. ────────────────────────────
// NOT part of the public API: include "cards/CardParser.hpp" instead.
#include <cstdint>
#include <nlohmann/json.hpp>
#include <string>

namespace openjoey::cards::detail {
// ─────────────────────────────────────────────────────────────────────────────
// ─────────────────────────── PARSER UTILITIES ───────────────────────────────
// ─────────────────────────────────────────────────────────────────────────────
// Remote providers occasionally encode stats as "?" or strings; anything
// unparseable maps to 0.
//
// Definition order (callee before caller):
//   parseStatField, optIntMember, optStringMember, optCardId  ←  cardFromRemoteJson
//
int parseStatField(const nlohmann::json &j, const char *key);
int optIntMember(const nlohmann::json &j, const char *key, int def = 0);
std::string optStringMember(const nlohmann::json &j, const char *key, const std::string &def = {});
uint32_t optCardId(const nlohmann::json &j);
Card cardFromRemoteJson(const nlohmann::json &j);
}  // namespace openjoey::cards::detail
