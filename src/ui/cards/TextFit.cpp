#include "ui/cards/TextFit.hpp"

namespace openjoey::ui {

std::string fitText(const std::string &text, int maxWidth, int fontSize) {
    std::string out = text;
    while (!out.empty() && MeasureText(out.c_str(), fontSize) > maxWidth) out.pop_back();
    if (out.size() < text.size()) out += "~";
    return out;
}

std::vector<std::string> wrapText(const std::string &text, int maxWidth, int fontSize, int maxLines) {
    std::vector<std::string> lines;
    std::string word, line;
    auto flushWord = [&]() {
        if (word.empty()) return;
        std::string candidate = line.empty() ? word : line + " " + word;
        if (MeasureText(candidate.c_str(), fontSize) <= maxWidth) {
            line = candidate;
        } else if (line.empty()) {
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
    if (!line.empty()) lines.push_back(line);

    if ((int)lines.size() > maxLines) {
        lines.resize(maxLines);
        if (!lines.empty()) lines.back() += "~";
    }
    return lines;
}

}  // namespace openjoey::ui
