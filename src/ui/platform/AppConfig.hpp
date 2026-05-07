#pragma once

namespace openjoey::ui {

struct AppConfig {
  int screenWidth = 1080;
  int screenHeight = 720;
  int targetFps = 60;
  const char *windowTitle = "OpenJoey";
};

} // namespace openjoey::ui
