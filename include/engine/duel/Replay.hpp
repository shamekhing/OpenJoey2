#pragma once
// ── Replay (duel/Replay.hpp) — deterministically re-run a recorded duel ─────
// Contract: a replay replays the SAME verb through the SAME Engine facade the
// recording used, then asserts duel.stateHash() equals the recorded hash after
// every record. Any mismatch names the exact record index — a determinism bug
// (unseeded RNG, unordered catalog iteration, a write that bypassed commit()).
// In-memory pointers inside ActionArgs are valid only against the same deck
// objects the duel was set up with; durable files map Card* -> card id.
#include <string>
#include <vector>

#include "engine/duel/Engine.hpp"
#include "engine/duel/Recorder.hpp"

namespace openjoey::engine {

struct ReplayHeader {
    uint32_t seed = 0;
    int firstPlayer = 0;
    DuelConfig config;                 // the ruleset the duel was recorded under
    std::vector<Card *> decks[2];      // app-owned deck vectors (pointer seal applies)
};

struct ReplayLog {
    ReplayHeader header;
    std::vector<DuelRecord> records;
};

struct ReplayResult {
    bool ok = false;
    size_t checked = 0;    // records replayed
    size_t failedAt = 0;   // index of the first mismatch (== checked on success)
    std::string msg;
};

ReplayResult replay(Engine &e, Duel &d, const ReplayLog &log);

}  // namespace openjoey::engine
