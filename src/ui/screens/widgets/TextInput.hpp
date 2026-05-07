#include "ui/StyleSheet.hpp"
#include <raylib.h>
#include <string>

#define isInputChar(c) ((c >= 32 && c < 127) || c == '\b')

namespace openjoey::ui {

class TextInput {
public:
  TextInput() = default;
  void Update();
  void Draw(int x, int y, int w, int h) const;
  std::string GetText() const { return text_; }
  bool isChanged() const { return changed_; }
  bool isTyping() const { return typing_; }

private:
  std::string text_ = "Right click to search...";
  bool typing_ = false;
  bool changed_ = false;
};

void TextInput::Update() {
  if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
    typing_ = !typing_;
    text_ = "";
  }

  changed_ = false;

  if (typing_) {
    int ch = GetCharPressed();
    while (ch > 0) {
      if (isInputChar(ch)) {
        text_ += (char)ch;
        changed_ = true;
      }
      ch = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !text_.empty()) {
      text_.pop_back();
      changed_ = true;
    }

    if (IsKeyPressed(KEY_ESCAPE)) {
      typing_ = false;
    }
  }
}

void TextInput::Draw(int x, int y, int w, int h) const {

  Color col = typing_ ? YELLOW : GRAY;
  DrawRectangleLines(x + TEXT_PAD(h), y, w - TEXT_PAD(h) * 2, h,
                     COLOR_FOCUS(typing_)); // Border

  std::string displayText = text_ + (typing_ ? "_" : "");

  int clipWidth = CLIP_WIDTH(displayText.c_str(), w, h);

  if (clipWidth > 0) {
    displayText = text_.substr(0, text_.size() - clipWidth) + "...";
  }

  DrawText(displayText.c_str(), x + TEXT_PAD(h) * 2, y + 2, TEXT_FONT_SIZE,
           col);
}
} // namespace openjoey::ui