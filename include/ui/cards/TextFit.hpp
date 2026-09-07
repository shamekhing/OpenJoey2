#pragma once
#include <raylib.h>
#include <string>
#include <vector>

namespace openjoey::ui {

// Truncates `text` character by character until it fits `maxWidth` at
// `fontSize`, appending "~" when characters were dropped.
inline std::string fitText(const std::string &text, int maxWidth, int fontSize) {
  std::string out = text;
  while (!out.empty() && MeasureText(out.c_str(), fontSize) > maxWidth)
    out.pop_back();
  if (out.size() < text.size())
    out += "~";
  return out;
}

// Word-wraps `text` so every line fits `maxWidth` at `fontSize`. Lines are
// split on spaces (never mid-word); a single word longer than the width is
// truncated with fitText. Returns at most `maxLines` lines, the last one
// ending with "~" when content was dropped.
inline std::vector<std::string>
wrapText(const std::string &text, int maxWidth, int fontSize, int maxLines) {
  std::vector<std::string> lines;
  std::string word, line;
  auto flushWord = [&]() {
    if (word.empty())
      return;
    std::string candidate = line.empty() ? word : line + " " + word;
    if (MeasureText(candidate.c_str(), fontSize) <= maxWidth) {
      line = candidate;
    } else if (line.empty()) {
      // Single word wider than the box — hard-truncate it.
      lines.push_back(fitText(word, maxWidth, fontSize));
      line.clear();
    } else {
      lines.push_back(line);
      line = word;
      if (MeasureText(line.c_str(), fontSize) > maxWidth) {
        lines.back() = fitText(line, maxWidth, fontSize);
        line.clear();
      }
    }
    word.clear();
  };
  for (char c : text) {
    if (c == '\n' || c == ' ') {
      flushWord();
      if (c == '\n' && !line.empty()) {
        lines.push_back(line);
        line.clear();
      }
    } else {
      word += c;
    }
  }
  flushWord();
  if (!line.empty())
    lines.push_back(line);

  if ((int)lines.size() > maxLines) {
    lines.resize(maxLines);
    if (!lines.empty())
      lines.back() += "~";
  }
  return lines;
}

} // namespace openjoey::ui
