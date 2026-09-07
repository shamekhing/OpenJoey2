// OpenJoey — entry point (openjoey::app shell)
#include "ui/core/App.hpp"

namespace openjoey::app {

// The application shell: boots the UI layer with the resolved config.
inline int run(int argc, char** argv) {
    // argv[0] lets Config resolve data/ beside the executable (the build dir
    // symlinks foundation data/), so the app runs from any working dir.
    openjoey::ui::App app(argc > 0 ? argv[0] : nullptr);
    app.Run();
    return 0;
}

} // namespace openjoey::app

int main(int argc, char** argv) {
    return openjoey::app::run(argc, argv);
}
