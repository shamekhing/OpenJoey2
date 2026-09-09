#pragma once
#include <raylib.h>

#include "Config.hpp"

namespace openjoey::ui {

class PlatformContext {
   public:
    explicit PlatformContext(const Config &config) : config_(config) {
        SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT | (config_.fullscreen ? FLAG_FULLSCREEN_MODE : 0));
        InitWindow(config_.screenWidth, config_.screenHeight, config_.windowTitle);
        SetExitKey(KEY_NULL);
        SetTargetFPS(config_.targetFps);
    }
    ~PlatformContext() { CloseWindow(); }

    PlatformContext(const PlatformContext &) = delete;
    PlatformContext &operator=(const PlatformContext &) = delete;

    bool ShouldClose() const { return WindowShouldClose(); }
    float FrameTime() const { return GetFrameTime(); }

   private:
    Config config_;
};

}  // namespace openjoey::ui