#pragma once
#include <raylib.h>

#include <string>
#include <vector>

namespace openjoey::ui {

// Truncates `text` character by character until it fits `maxWidth` at
// `fontSize`, appending "~" when characters were dropped.
std::string fitText(const std::string &text, int maxWidth, int fontSize);

// Word-wraps `text` so every line fits `maxWidth` at `fontSize`. Lines are
// split on spaces (never mid-word); a single word longer than the width is
// truncated with fitText. Returns at most `maxLines` lines, the last one
// ending with "~" when content was dropped.
std::vector<std::string> wrapText(const std::string &text, int maxWidth, int fontSize, int maxLines);

}  // namespace openjoey::ui
