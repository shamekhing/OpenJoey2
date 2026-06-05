#pragma once
#include "ContentPaths.hpp"
#include "ui/core/Event.hpp"
#include <filesystem>
#include <raylib.h>

namespace openjoey::ui {

class IScreen {
public:
  virtual ~IScreen() = default;

  // Handle input and update state. Returns a ScreenEvent signalling
  // transitions or quit — no separate NextScreen() polling needed.
  virtual ScreenEvent Update(float dt) = 0;

  // Render the screen. Window framing (BeginDrawing/EndDrawing) is
  // managed by App::Run(), not by the screen.
  virtual void Draw() const = 0;

  virtual void loadBackground(std::filesystem::path path) {
    if (std::filesystem::exists(path))
      background_ = LoadTexture(path.c_str());
  };

  virtual void unloadBackground() {
    if (background_.id)
      UnloadTexture(background_);
  };

protected:
  Texture2D background_ = {};
};

} // namespace openjoey::ui
