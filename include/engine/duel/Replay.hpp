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

inline ReplayResult replay(Engine &e, Duel &d, const ReplayLog &log) {
    ReplayResult out;
    // Replay must not feed the recorder: set up silently.
    e.setRecording(false);
    e.hardReset();

    d.seedRng(log.header.seed);
    d.config = log.header.config;
    d.turnPlayer = log.header.firstPlayer;

    e.setDeck(0, log.header.decks[0]);
    e.setDeck(1, log.header.decks[1]);
    e.shuffleDecks();
    e.drawOpeningHands();
    d.turnPlayer = log.header.firstPlayer;  // re-assert: setup must not move it

    for (const DuelRecord &rec : log.records) {
        const std::string &v = rec.verb;
        ActionResult r;
        if (v == "startTurn") r = e.startTurn();
        else if (v == "endTurn") r = e.endTurn();
        else if (v == "toMain1") r = e.toMain1();
        else if (v == "toMain2") r = e.toMain2();
        else if (v == "toBattle") r = e.toBattle();
        else if (v == "declareAttack") r = e.declareAttack(rec.args.source, rec.args.target);
        else if (v == "cancelAttack") e.cancelAttack();
        else if (v == "resolveDamage") r = e.resolveDamage();
        else if (v == "normalSummon") r = e.normalSummon(rec.args.target);
        else if (v == "normalSet") r = e.normalSet(rec.args.target);
        else if (v == "tributeSummon") r = e.tributeSummon(rec.args.target, rec.args.materials);
        else if (v == "flipSummon") r = e.flipSummon(rec.args.target);
        else if (v == "changePosition") r = e.changePosition(rec.args.target);
        else if (v == "fusionSummon") r = e.fusionSummon(rec.args.target, rec.args.materials);
        else if (v == "ritualSummon") r = e.ritualSummon(rec.args.target, rec.args.materials);
        else if (v == "setSpellTrap") r = e.setSpellTrap(rec.args.target, rec.id);
        else if (v == "activateEffect") r = e.activateEffect(rec.args.spec, rec.activator, rec.args);
        else if (v == "passResponse") r = e.passResponse(rec.activator);
        else if (v == "resolveChain") r = e.resolveChain();
        else {
            out.msg = "unknown verb '" + v + "'";
            out.failedAt = out.checked;
            return out;
        }

        if (!v.empty() && v != "cancelAttack") {
            if (r.ok != rec.result.ok) {
                out.msg = "verdict mismatch on '" + v + "' (recorded " + (rec.result.ok ? "ok" : "fail") + ", replayed " + (r.ok ? "ok" : "fail") + "): " + r.msg;
                out.failedAt = out.checked;
                return out;
            }
        }
        const uint32_t h = d.stateHash();
        if (h != rec.hash) {
            out.msg = "state hash mismatch on '" + v + "' — determinism bug (desync)";
            out.failedAt = out.checked;
            return out;
        }
        ++out.checked;
    }
    out.ok = true;
    out.failedAt = out.checked;
    out.msg = "replayed " + std::to_string(out.checked) + " records, all hashes match.";
    return out;
}

}  // namespace openjoey::engine
