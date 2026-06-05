#pragma once
#include "ui/core/AppScreen.hpp"

namespace openjoey::ui {

// Returned by IScreen::Update() to signal navigation without polling.
struct ScreenEvent {
    enum class Type { None, Replace, Quit };
    Type      type   = Type::None;
    AppScreen target = AppScreen::MainMenu;

    static ScreenEvent none()                { return {}; }
    static ScreenEvent replace(AppScreen s)  { return {Type::Replace, s}; }
    static ScreenEvent quit()                { return {Type::Quit}; }
};

} // namespace openjoey::ui
