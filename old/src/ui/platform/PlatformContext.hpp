#pragma once
#include "AppConfig.hpp"
#include <raylib.h>

namespace openjoey::ui {

class PlatformContext {
public:
  explicit PlatformContext(const AppConfig &config) : config_(config) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT);
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
  AppConfig config_;
};

} // namespace openjoey::ui