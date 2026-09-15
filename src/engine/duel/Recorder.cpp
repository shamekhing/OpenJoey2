// Recorder implementation (duel/Recorder.cpp).
#include "engine/duel/Recorder.hpp"

namespace openjoey::engine {

void MemoryRecorder::onAction(const DuelRecord &r) { records.push_back(r); }

void MemoryRecorder::onKeyframe(DuelKeyframe k) { keyframes.push_back(std::move(k)); }

void MemoryRecorder::onReset() {
    records.clear();
    keyframes.clear();
}

const std::vector<DuelRecord> &MemoryRecorder::log() const { return records; }

const std::vector<DuelKeyframe> &MemoryRecorder::frames() const { return keyframes; }

size_t MemoryRecorder::size() const { return records.size(); }

}  // namespace openjoey::engine
