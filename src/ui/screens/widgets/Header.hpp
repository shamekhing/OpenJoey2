#pragma once
#include "ui/widgets/layout/Panel.hpp"
#include <string>

namespace openjoey::ui {
struct Header {
    static void Draw(const std::string& title, const std::string& badge,
                     int x, int y, int w, int h, bool focused) {
        Panel::Draw(title.c_str(), badge.empty() ? nullptr : badge.c_str(),
                    x, y, w, h, focused);
    }
    static void Draw(const std::string& title,
                     int x, int y, int w, int h, bool focused) {
        Panel::Draw(title.c_str(), x, y, w, h, focused);
    }
};
} // namespace openjoey::ui
