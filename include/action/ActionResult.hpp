#pragma once
// ── ActionResult — THE structured engine verdict (openjoey) ─────────────────
// Every action the engine performs or refuses returns one. ok=false always
// carries the rules reason in msg; ok=true carries the narration. This
// replaces string-only verdicts, which made success indistinguishable from
// failure at every call site (the UI string-sniffed for "OK"/"true"/"Moved").
#include <string>
#include <utility>

#include "action/ActionId.hpp"

namespace openjoey {

struct ActionResult {
    bool ok = false;
    ActionId id = ActionId::None;
    std::string msg;

    ActionResult() = default;
    ActionResult(bool succeeded, std::string m, ActionId i = ActionId::None) : ok(succeeded), id(i), msg(std::move(m)) {}

    static ActionResult Ok(std::string m, ActionId i = ActionId::None) { return {true, std::move(m), i}; }
    static ActionResult Fail(std::string m, ActionId i = ActionId::None) { return {false, std::move(m), i}; }
    explicit operator bool() const { return ok; }
};

}  // namespace openjoey
