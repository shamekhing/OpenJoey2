#pragma once

#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

namespace openjoey::ui {

// ── Content paths (replaces the deleted ContentPaths.hpp) ────────────────────
// Paths & URLs now live in the serialized Settings so they are user-overridable
// through data/user_settings.json instead of being hard-coded.
struct Settings {
    struct Paths {
        std::filesystem::path cardsJson;
        std::filesystem::path banlistJson;
        std::filesystem::path cardImgDir;
        std::filesystem::path cardBackImg;
        std::string ygoprodeckUrl;
        std::string ygoprodeckUrlSmall;

        // Fill in the standard data/ layout relative to the working dir.
        void defaults() {
            namespace fs = std::filesystem;
            fs::path base = fs::current_path() / "data";
            cardsJson        = base / "cards.json";
            banlistJson      = base / "banlist.json";
            cardImgDir       = base / "images";
            cardBackImg      = base / "card_back.png";
            ygoprodeckUrl        = "https://images.ygoprodeck.com/images/cards/";
            ygoprodeckUrlSmall   = "https://images.ygoprodeck.com/images/cards_small/";
        }
        } paths;

    // Default-constructed settings carry valid on-disk path defaults so that
    // even a bare `Settings s;` is safe to inspect (e.g. in tests, or before Load).
    Settings() { paths.defaults(); }

    int  screenWidth    = 1620;
    int  screenHeight   = 920;
    bool fullscreen     = false;
    int  targetFps      = 60;
    bool downloadImages = true;

    // Where the settings file itself lives (was ContentPaths::userSettingsJson).
    static std::filesystem::path settingsFile() {
        return std::filesystem::current_path() / "data" / "user_settings.json";
    }

    static Settings Load() {
        Settings s;
        s.paths.defaults();
        std::error_code ec;
        auto path = settingsFile();
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
            if (j.contains("paths") && j["paths"].is_object()) {
                auto& pp = j["paths"];
                if (pp.contains("cardsJson"))          s.paths.cardsJson = pp["cardsJson"].get<std::string>();
                if (pp.contains("banlistJson"))        s.paths.banlistJson = pp["banlistJson"].get<std::string>();
                if (pp.contains("cardImgDir"))         s.paths.cardImgDir = pp["cardImgDir"].get<std::string>();
                if (pp.contains("cardBackImg"))        s.paths.cardBackImg = pp["cardBackImg"].get<std::string>();
                if (pp.contains("ygoprodeckUrl"))      s.paths.ygoprodeckUrl = pp["ygoprodeckUrl"].get<std::string>();
                if (pp.contains("ygoprodeckUrlSmall")) s.paths.ygoprodeckUrlSmall = pp["ygoprodeckUrlSmall"].get<std::string>();
            }
        } catch (const std::exception& e) {
            std::cerr << "[Settings] failed to parse " << path << ": " << e.what() << "\n";
        }
        return s;
    }

    bool Save() const {
        std::error_code ec;
        auto path = settingsFile();
        std::filesystem::create_directories(path.parent_path(), ec);
        nlohmann::json j;
        j["screenWidth"]    = screenWidth;
        j["screenHeight"]   = screenHeight;
        j["fullscreen"]     = fullscreen;
        j["targetFps"]      = targetFps;
        j["downloadImages"] = downloadImages;
        j["paths"]["cardsJson"]          = paths.cardsJson.string();
        j["paths"]["banlistJson"]        = paths.banlistJson.string();
        j["paths"]["cardImgDir"]         = paths.cardImgDir.string();
        j["paths"]["cardBackImg"]        = paths.cardBackImg.string();
        j["paths"]["ygoprodeckUrl"]      = paths.ygoprodeckUrl;
        j["paths"]["ygoprodeckUrlSmall"] = paths.ygoprodeckUrlSmall;
        std::ofstream out(path);
        if (!out) return false;
        out << std::setw(2) << j;
        return true;
    }
};

} // namespace openjoey::ui
