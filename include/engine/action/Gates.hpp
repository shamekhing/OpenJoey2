#pragma once
#include "engine/action/State.hpp"
namespace openjoey::engine::action {
inline std::string SynchroGate(Duel &d) { (void)d; return ClassicGate("Synchro Summons"); }
inline std::string XyzGate(Duel &d) { (void)d; return ClassicGate("Xyz Summons"); }
inline std::string PendulumGate(Duel &d) { (void)d; return ClassicGate("Pendulum Summons"); }
inline std::string LinkGate(Duel &d) { (void)d; return ClassicGate("Link Summons"); }
} // namespace openjoey::engine::action
