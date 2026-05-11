#pragma once

namespace openjoey::ui {

struct AppConfig {
  int screenWidth = 1620;
  int screenHeight = 920;
  int targetFps = 60;
  const char *windowTitle = "OpenJoey";
};

} // namespace openjoey::ui
