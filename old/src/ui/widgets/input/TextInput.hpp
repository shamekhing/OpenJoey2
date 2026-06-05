#pragma once
#include <raylib.h>
#include <string>

namespace openjoey::ui {

class TextInput {
public:
    TextInput() = default;
    void Update();
    void Draw(int x, int y, int w, int h) const;
    std::string GetText() const { return text_; }
    bool isChanged() const { return changed_; }
    bool isTyping()  const { return typing_;  }

private:
    static bool isInputChar(int c) { return (c >= 32 && c < 127) || c == '\b'; }

    std::string text_    = "Right click to search...";
    bool        typing_  = false;
    bool        changed_ = false;
};

inline void TextInput::Update() {
    if (IsMouseButtonPressed(MOUSE_RIGHT_BUTTON)) {
        typing_ = !typing_;
        text_   = "";
    }
    changed_ = false;
    if (typing_) {
        int ch = GetCharPressed();
        while (ch > 0) {
            if (isInputChar(ch)) { text_ += (char)ch; changed_ = true; }
            ch = GetCharPressed();
        }
        if (IsKeyPressed(KEY_BACKSPACE) && !text_.empty()) {
            text_.pop_back();
            changed_ = true;
        }
        if (IsKeyPressed(KEY_ESCAPE))
            typing_ = false;
    }
}

inline void TextInput::Draw(int x, int y, int w, int h) const {
    int charW = MeasureText("W", int(0.8f * h));
    int pad   = charW;
    Color border = typing_ ? YELLOW : GRAY;
    DrawRectangleLines(x + pad, y, w - pad * 2, h, border);

    std::string display = text_ + (typing_ ? "_" : "");
    int textW = MeasureText(display.c_str(), int(0.8f * h));
    int avail = w - pad * 4;
    if (textW > avail) {
        int overflow = (textW - avail) / std::max(1, charW);
        size_t trim = (size_t)overflow < text_.size() ? text_.size() - overflow : 0;
        display = text_.substr(0, trim) + "...";
    }
    int fs = int(0.04f * GetScreenHeight());
    DrawText(display.c_str(), x + pad * 2, y + 2, fs, typing_ ? YELLOW : GRAY);
}

} // namespace openjoey::ui
