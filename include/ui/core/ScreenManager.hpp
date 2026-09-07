#pragma once
#include "ui/screens/IScreen.hpp"
#include <memory>
#include <vector>

namespace openjoey::ui {

// Owns the active screen stack. Replace() swaps the top screen, supporting
// transitions without nested loops or a switch in App::Run().
class ScreenManager {
public:
    void Replace(std::unique_ptr<IScreen> screen) {
        if (!stack_.empty())
            stack_.pop_back();
        stack_.push_back(std::move(screen));
    }

    bool Empty() const { return stack_.empty(); }

    IScreen& Top() { return *stack_.back(); }

private:
    std::vector<std::unique_ptr<IScreen>> stack_;
};

} // namespace openjoey::ui
