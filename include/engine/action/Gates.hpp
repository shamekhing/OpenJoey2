#pragma once
#include "engine/action/State.hpp"
namespace openjoey::engine::action {

using openjoey::ActionResult;

inline ActionResult SynchroGate(Duel &d) {
    (void)d;
    return ClassicGate("Synchro Summons");
}
inline ActionResult XyzGate(Duel &d) {
    (void)d;
    return ClassicGate("Xyz Summons");
}
inline ActionResult PendulumGate(Duel &d) {
    (void)d;
    return ClassicGate("Pendulum Summons");
}
inline ActionResult LinkGate(Duel &d) {
    (void)d;
    return ClassicGate("Link Summons");
}
}  // namespace openjoey::engine::action
