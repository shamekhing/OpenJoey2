#pragma once
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <nlohmann/json.hpp>
#include <string>

namespace openjoey {

// ── Config: the ONE application-configuration type ───────────────────────────
// Everything tunable lives here exactly once: content paths, remote provider
// endpoints (config-supplied, never baked into code), and window/app options
// (absorbed from the old AppConfig — no duplicated field pairs). Mirrors the
// nested schema of data/settings.json:
//
//   { "file": { "cardsJson", "banlistJson", "cardBackImg" },
//     "dir":  { "cardImgDir" },
//     "url":  { "cardsJsonUrl", "cardImgUrl", "cardImgSmallUrl" },
//     "app":  { "screenWidth", "screenHeight", "fullscreen", "targetFps",
//               "downloadImages" } }
//
// Two files, two roles:
//   * data/settings.json       — shipped-defaults reference (in git); Load()
//                                applies it first.
//   * data/user_settings.json  — app-written overrides (gitignored); Save()
//                                writes it, Load() applies it last. The
//                                pre-0.2 flat layout is still honored.
struct Config {
    struct Paths {
        std::filesystem::path cardsJson;
        std::filesystem::path banlistJson;
        std::filesystem::path cardImgDir;
        std::filesystem::path cardBackImg;
        std::string cardsJsonUrl;     // remote card-database endpoint (content config)
        std::string cardImgUrl;       // full card art base URL
        std::string cardImgSmallUrl;  // small card art base URL (fallback)

        // Fill in the standard data/ layout under the given base dir.
        void defaults(const std::filesystem::path& base) {
            cardsJson = base / "cards.json";
            banlistJson = base / "banlist.json";
            cardImgDir = base / "images";
            cardBackImg = base / "card_back.png";
            // URL endpoints are intentionally NOT baked into the code — they are
            // content-layer configuration (data/settings.json, `url` group).
        }
    } paths;

    // ── Window / app options (absorbed AppConfig; persisted under "app") ──────
    int screenWidth = 1620;
    int screenHeight = 920;
    bool fullscreen = false;
    int targetFps = 60;
    bool downloadImages = true;

    // ── Duel format switches (per-duel engine config; defaults = shipped classic) ─
    bool chainResponseWindow = false;  // p.45: chains resolve only after both pass
    bool autoDiscardEndPhase = true;   // p.41: EndTurn auto-discards down to 6
    const char* windowTitle = "OpenJoey";

    // Default-constructed config carries valid on-disk path defaults so that
    // even a bare `Config c;` is safe to inspect (tests, or before Load).
    Config() {
        baseDir_ = std::filesystem::current_path() / "data";
        paths.defaults(baseDir_);
    }

    // Directory that holds user_settings.json (the effective data dir).
    std::filesystem::path baseDir_;

    // Directory of the running executable (argv[0]'s folder); falls back to
    // the CWD when argv0 is missing or unresolvable.
    static std::filesystem::path exeDir(const char* argv0) {
        namespace fs = std::filesystem;
        if (!argv0 || !*argv0) return fs::current_path();
        std::error_code ec;
        fs::path p = fs::weakly_canonical(fs::path(argv0), ec);
        if (ec || p.empty()) p = fs::path(argv0);
        if (p.is_relative()) p = fs::absolute(p, ec);
        fs::path dir = p.parent_path();
        return dir.empty() ? fs::current_path() : dir;
    }

    // Resolve `name` inside the effective data dir: prefer the one next to
    // the executable (the app build dir symlinks foundation data/), else
    // the CWD (tests, CLI runs). The exe-side file wins when both exist.
    static std::filesystem::path resolveDataFile(const char* argv0, const char* name) {
        namespace fs = std::filesystem;
        fs::path beside = exeDir(argv0) / "data" / name;
        if (std::filesystem::exists(beside)) return beside;
        fs::path inCwd = fs::current_path() / "data" / name;
        if (std::filesystem::exists(inCwd)) return inCwd;
        return beside;  // default creation target: beside the executable
    }

    // Shipped-defaults reference (in git): data/settings.json — Load() layer 1.
    static std::filesystem::path referenceFile(const char* argv0) { return resolveDataFile(argv0, "settings.json"); }

    // App-written overrides (gitignored): data/user_settings.json — what
    // Save() produces and Load() applies last (layer 2).
    static std::filesystem::path settingsFile(const char* argv0) { return resolveDataFile(argv0, "user_settings.json"); }

    std::filesystem::path settingsFile() const { return baseDir_ / "user_settings.json"; }

    // ── JSON plumbing: nested settings.json schema + legacy flat fallback ────
    static const nlohmann::json* findKey(const nlohmann::json& j, const char* group, const char* key) {
        const nlohmann::json* scope = &j;
        if (group) {
            auto g = j.find(group);
            if (g == j.end() || !g->is_object()) return nullptr;
            scope = &*g;
        }
        auto it = scope->find(key);
        return it != scope->end() ? &*it : nullptr;
    }

    template <typename T>
    static T pick(const nlohmann::json& j, const char* group, const char* key, T fallback) {
        if (auto v = findKey(j, group, key)) return v->get<T>();
        return fallback;
    }

    // Path values are written relative to the content root ("data/cards.json")
    // or to the data dir itself (bare overrides). Absolute values win; a
    // leading "data/" segment is content-root relative and collapses onto
    // baseDir.
    static std::filesystem::path resolveEntry(const std::string& raw, const std::filesystem::path& baseDir) {
        namespace fs = std::filesystem;
        if (raw.empty()) return {};
        fs::path p(raw);
        if (p.is_absolute()) return p;
        const std::string dataName = baseDir.filename().string();
        fs::path rest;
        bool first = true;
        for (auto it = p.begin(); it != p.end(); ++it) {
            if (it->empty()) continue;                                                    // drop separator artifacts (e.g. "dir/")
            if (!(first && !dataName.empty() && *it == fs::path(dataName))) rest /= *it;  // content-root relative: skip "data"
            first = false;
        }
        return baseDir / rest;
    }

    // Overlay one settings file onto `c`. Malformed content warns on stderr
    // and is skipped — previously applied layers stay intact. Never throws.
    static void applyFile(Config& c, const std::filesystem::path& path) {
        std::ifstream in(path);
        if (!in) return;
        try {
            const auto j = nlohmann::json::parse(in);
            // Window / download options: nested `app` group, then legacy flat keys.
            c.screenWidth = pick(j, "app", "screenWidth", c.screenWidth);
            c.screenHeight = pick(j, "app", "screenHeight", c.screenHeight);
            c.fullscreen = pick(j, "app", "fullscreen", c.fullscreen);
            c.targetFps = pick(j, "app", "targetFps", c.targetFps);
            c.downloadImages = pick(j, "app", "downloadImages", c.downloadImages);
            c.chainResponseWindow = pick(j, "app", "chainResponseWindow", c.chainResponseWindow);
            c.autoDiscardEndPhase = pick(j, "app", "autoDiscardEndPhase", c.autoDiscardEndPhase);
            c.screenWidth = pick(j, nullptr, "screenWidth", c.screenWidth);
            c.screenHeight = pick(j, nullptr, "screenHeight", c.screenHeight);
            c.fullscreen = pick(j, nullptr, "fullscreen", c.fullscreen);
            c.targetFps = pick(j, nullptr, "targetFps", c.targetFps);
            c.downloadImages = pick(j, nullptr, "downloadImages", c.downloadImages);
            // Content paths: `file` / `dir` groups, then the legacy `paths` group.
            auto pathEntry = [&](const char* group, const char* key, std::filesystem::path& dst) {
                if (auto v = findKey(j, group, key)) dst = resolveEntry(v->get<std::string>(), c.baseDir_);
                else if (auto v2 = findKey(j, "paths", key)) dst = resolveEntry(v2->get<std::string>(), c.baseDir_);
            };
            pathEntry("file", "cardsJson", c.paths.cardsJson);
            pathEntry("file", "banlistJson", c.paths.banlistJson);
            pathEntry("dir", "cardImgDir", c.paths.cardImgDir);
            pathEntry("file", "cardBackImg", c.paths.cardBackImg);
            // URLs come from the `url` group — the content layer owns them.
            auto urlEntry = [&](const char* key, std::string& dst) {
                if (auto v = findKey(j, "url", key)) dst = v->get<std::string>();
            };
            urlEntry("cardsJsonUrl", c.paths.cardsJsonUrl);
            urlEntry("cardImgUrl", c.paths.cardImgUrl);
            urlEntry("cardImgSmallUrl", c.paths.cardImgSmallUrl);
        } catch (const std::exception& e) { std::cerr << "[Config] failed to load " << path << ": " << e.what() << "\n"; }
    }

    // Layered load: compiled-in defaults → data/settings.json (shipped
    // reference) → data/user_settings.json (app-written overrides, wins).
    static Config Load(const char* argv0 = nullptr) {
        Config c;  // compiled-in defaults, <cwd>/data base dir
        std::error_code ec;
        auto reference = referenceFile(argv0);
        if (std::filesystem::exists(reference, ec)) {
            c.baseDir_ = reference.parent_path();
            c.paths.defaults(c.baseDir_);  // re-derive relative defaults here
            applyFile(c, reference);
        }
        auto user = settingsFile(argv0);
        if (std::filesystem::exists(user, ec)) {
            c.baseDir_ = user.parent_path();  // Save() keeps writing beside them
            applyFile(c, user);               // pure overlay: nothing re-defaulted
        }
        return c;
    }

    // Serialize to data/user_settings.json (gitignored) in the same nested
    // schema as data/settings.json; Load() reads it back verbatim.
    bool Save() const {
        std::error_code ec;
        auto path = settingsFile();
        std::filesystem::create_directories(path.parent_path(), ec);
        nlohmann::json j;
        j["app"]["screenWidth"] = screenWidth;
        j["app"]["screenHeight"] = screenHeight;
        j["app"]["fullscreen"] = fullscreen;
        j["app"]["targetFps"] = targetFps;
        j["app"]["downloadImages"] = downloadImages;
        j["app"]["chainResponseWindow"] = chainResponseWindow;
        j["app"]["autoDiscardEndPhase"] = autoDiscardEndPhase;
        j["file"]["cardsJson"] = paths.cardsJson.string();
        j["file"]["banlistJson"] = paths.banlistJson.string();
        j["file"]["cardBackImg"] = paths.cardBackImg.string();
        j["dir"]["cardImgDir"] = paths.cardImgDir.string();
        j["url"]["cardsJsonUrl"] = paths.cardsJsonUrl;
        j["url"]["cardImgSmallUrl"] = paths.cardImgSmallUrl;
        j["url"]["cardImgUrl"] = paths.cardImgUrl;
        std::ofstream out(path);
        if (!out) return false;
        out << std::setw(2) << j;
        return true;
    }
};

}  // namespace openjoey
