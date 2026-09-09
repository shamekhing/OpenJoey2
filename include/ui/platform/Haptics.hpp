#pragma once
// ── Haptics — mobile-web vibration pulse (openjoey::ui::platform) ────────────
// A tiny verdict "thud" when the engine accepts or refuses an action. Android
// Chrome supports navigator.vibrate; iOS Safari does not (the guard no-ops).
// Native builds compile to nothing.
#if defined(__EMSCRIPTEN__)
#include <emscripten.h>

namespace openjoey::ui::platform {

inline void hapticPulse(int ms) {
    EM_ASM(
        {
            if (navigator.vibrate) navigator.vibrate($0);
        },
        ms);
}

}  // namespace openjoey::ui::platform
#else

namespace openjoey::ui::platform {

inline void hapticPulse(int) {}

}  // namespace openjoey::ui::platform
#endif