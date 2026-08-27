#pragma once
#include "ContentPaths.hpp"

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>

namespace openjoey::ui {

// Persistent user preferences stored as JSON in data/user_settings.json.
// Defaults intentionally match the values the app previously hard-coded.
struct Settings {
    int  screenWidth    = 1620;
    int  screenHeight   = 920;
    bool fullscreen     = false;
    int  targetFps      = 60;
    bool downloadImages = true;

    static Settings Load() {
        Settings s;
        std::error_code ec;
        auto path = openjoey::ContentPaths::userSettingsJson();
        if (!std::filesystem::exists(path, ec)) return s;
        std::ifstream in(path);
        if (!in) return s;
        try {
            auto j = nlohmann::json::parse(in);
            s.screenWidth    = j.value("screenWidth", s.screenWidth);
            s.screenHeight   = j.value("screenHeight", s.screenHeight);
            s.fullscreen     = j.value("fullscreen", s.fullscreen);
            s.targetFps      = j.value("targetFps", s.targetFps);
            s.downloadImages = j.value("downloadImages", s.downloadImages);
        } catch (const std::exception& e) {
            std::cerr << "[Settings] failed to parse " << path << ": " << e.what() << "\n";
        }
        return s;
    }

    bool Save() const {
        std::error_code ec;
        auto path = openjoey::ContentPaths::userSettingsJson();
        std::filesystem::create_directories(path.parent_path(), ec);
        nlohmann::json j;
        j["screenWidth"]    = screenWidth;
        j["screenHeight"]   = screenHeight;
        j["fullscreen"]     = fullscreen;
        j["targetFps"]      = targetFps;
        j["downloadImages"] = downloadImages;
        std::ofstream out(path);
        if (!out) return false;
        out << std::setw(2) << j;
        return true;
    }
};

} // namespace openjoey::ui
