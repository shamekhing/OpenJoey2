#pragma once
#include "ui/core/Event.hpp"

namespace openjoey::ui {

class IScreen {
public:
    virtual ~IScreen() = default;

    // Handle input and update state. Returns a ScreenEvent signalling
    // transitions or quit — no separate NextScreen() polling needed.
    virtual ScreenEvent Update(float dt) = 0;

    // Render the screen. Window framing (BeginDrawing/EndDrawing) is
    // managed by App::Run(), not by the screen.
    virtual void Draw() const = 0;
};

} // namespace openjoey::ui
