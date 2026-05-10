#pragma once

namespace openjoey::ui {

struct AppConfig {
  int screenWidth = 1920;
  int screenHeight = 1080;
  int targetFps = 60;
  const char *windowTitle = "OpenJoey";
};

} // namespace openjoey::ui
