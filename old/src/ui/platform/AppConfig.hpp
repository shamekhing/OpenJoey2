#pragma once

namespace openjoey::ui {

struct AppConfig {
  int screenWidth = 1200;
  int screenHeight = 900;
  int targetFps = 60;
  const char *windowTitle = "OpenJoey";
};

} // namespace openjoey::ui
