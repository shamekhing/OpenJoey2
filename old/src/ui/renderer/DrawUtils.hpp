#pragma once
#include <algorithm>
#include <raylib.h>
#include <string>

// Stateless drawing helpers used by multiple widgets.
// Consolidates blitCard and clipText which were previously duplicated
// across ZoneCell, ZoneInfoPanel, and both preview panels.
namespace openjoey::ui {

struct DrawUtils {
    static constexpr float kCardAspect = 59.f / 86.f; // portrait W:H

    // Draw a card texture fitted into dst, optionally rotated 90° CW for DEF.
    static void blitCard(Rectangle dst, const Texture2D& tex, bool rotateDef) {
        Rectangle src = {0, 0, (float)tex.width, (float)tex.height};
        if (!rotateDef) {
            float rectAsp = dst.width / dst.height;
            Rectangle fit = dst;
            if (rectAsp > kCardAspect) {
                fit.width = dst.height * kCardAspect;
                fit.x = dst.x + (dst.width - fit.width) * 0.5f;
            } else {
                fit.height = dst.width / kCardAspect;
                fit.y = dst.y + (dst.height - fit.height) * 0.5f;
            }
            DrawTexturePro(tex, src, fit, {0, 0}, 0.f, WHITE);
        } else {
            float cx = dst.x + dst.width * 0.5f;
            float cy = dst.y + dst.height * 0.5f;
            float postAspect = 1.f / kCardAspect;
            float postW, postH;
            if (dst.width / dst.height >= postAspect) {
                postH = dst.height;
                postW = dst.height * postAspect;
            } else {
                postW = dst.width;
                postH = dst.width / postAspect;
            }
            float preW = postH, preH = postW;
            Rectangle d = {cx - preW * 0.5f, cy - preH * 0.5f, preW, preH};
            DrawTexturePro(tex, src, d, {preW * 0.5f, preH * 0.5f}, 90.f, WHITE);
        }
    }

    // Truncate s so it fits within maxPx pixels at font size fs, appending "..".
    static std::string clipText(const std::string& s, int maxPx, int fs) {
        if (MeasureText(s.c_str(), fs) <= maxPx) return s;
        std::string t = s;
        while (t.size() > 1 &&
               MeasureText((t + "..").c_str(), fs) > maxPx)
            t.pop_back();
        return t + "..";
    }

    // Word-wrap desc into lines no wider than maxPx at font size fs.
    static std::vector<std::string> wrapText(const std::string& desc,
                                             int maxPx, int fs) {
        std::vector<std::string> lines;
        size_t pos = 0;
        while (pos < desc.size()) {
            size_t end = pos;
            std::string line;
            while (end < desc.size()) {
                size_t nxt = desc.find(' ', end + 1);
                if (nxt == std::string::npos) nxt = desc.size();
                std::string trial = desc.substr(pos, nxt - pos);
                if (MeasureText(trial.c_str(), fs) > maxPx) break;
                line = trial;
                end  = nxt;
            }
            if (end == pos) { end = pos + 1; line = desc.substr(pos, 1); }
            lines.push_back(line);
            pos = (end < desc.size() && desc[end] == ' ') ? end + 1 : end;
        }
        return lines;
    }
};

} // namespace openjoey::ui
