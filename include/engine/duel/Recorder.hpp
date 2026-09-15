#pragma once
// ── Recorder (duel/Recorder.hpp) — append-only action log for duel replay ───
// The Engine facade reports every mutator through commit(); the recorder is a
// passive observer: it NEVER mutates the duel. Frame policy:
//   * one DuelRecord per recorded action (verb + args + verdict + state hash)
//   * one DuelKeyframe per completed turn (EndTurn OK) — the "1 dataframe per
//     turn" scrubbing anchor; the per-action deltas are the records themselves.
// Pointers inside ActionArgs are only valid for in-memory replay of the same
// deck objects; durable file serialization maps Card* -> card id (app side).
#include <string>
#include <vector>

#include "action/ActionArgs.hpp"
#include "action/ActionResult.hpp"
#include "engine/duel/Duel.hpp"
#include "engine/duel/Undo.hpp"

namespace openjoey::engine {

struct DuelRecord {
    std::string verb;      // the Engine facade method ("normalSummon", "endTurn", ...)
    int turnNumber = 0;
    int turnPlayer = 0;
    int activator = 0;     // who acted (may differ from turnPlayer: responses)
    ActionId id = ActionId::None;
    ActionArgs args;
    ActionResult result;
    uint32_t hash = 0;     // duel.stateHash() AFTER the action
};

struct DuelKeyframe {
    DuelSnapshot snapshot;
};

class IRecorder {
   public:
    virtual ~IRecorder() = default;
        virtual void onAction(const DuelRecord &) = 0;
    // Keyframes are MOVABLE only (DuelSnapshot owns unique_ptr tokens): take by
    // value so backends receive the deep-copy via move, never by const-ref copy.
    virtual void onKeyframe(DuelKeyframe) = 0;
    virtual void onReset() = 0;
};

// In-memory recorder: the contract every durable backend must satisfy.
class MemoryRecorder : public IRecorder {
   public:
    void onAction(const DuelRecord &r) override { records.push_back(r); }
    void onKeyframe(DuelKeyframe k) override { keyframes.push_back(std::move(k)); }
    void onReset() override { records.clear(); keyframes.clear(); }

    const std::vector<DuelRecord> &log() const { return records; }
    const std::vector<DuelKeyframe> &frames() const { return keyframes; }
    size_t size() const { return records.size(); }

   private:
    std::vector<DuelRecord> records;
    std::vector<DuelKeyframe> keyframes;
};

}  // namespace openjoey::engine
