#pragma once
#include <raylib.h>

namespace openjoey::ui {

// Base interface for all stateful UI components.
// Bounds are supplied per-call so widgets remain layout-agnostic.
class IWidget {
public:
    virtual ~IWidget() = default;
    virtual void Update(Rectangle bounds, float dt) = 0;
    virtual void Draw(Rectangle bounds) const = 0;
};

} // namespace openjoey::ui
