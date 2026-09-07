// openjoey-foundation unit tests: Config round-trip/path resolution and the
// ActionId catalog pins. Raylib-free: links openjoey::foundation only.
#define CATCH_CONFIG_RUNNER
#include "catch.hpp"

#include "Config.hpp"
#include "action/ActionId.hpp"

#include <cstdio>
#include <filesystem>
#include <string>

namespace fs = std::filesystem;
using openjoey::Config;

// Sandbox next to the test executable: every test reads/writes only
// <exe_dir>/core_test_sandbox/... so real user_settings.json files are never
// touched. Load() is exercised via a FAKE argv0 pointing inside the sandbox —
// Config::settingsFile(argv0) resolves data/ beside the "executable" path,
// which does not need to exist.
static fs::path g_sandbox;

static std::string fakeArgv0(const char* subdir) {
    return (g_sandbox / subdir / "app.exe").string();
}

int main(int argc, char* argv[]) {
    g_sandbox = Config::exeDir(argc > 0 ? argv[0] : nullptr) / "core_test_sandbox";
    std::error_code ec;
    fs::remove_all(g_sandbox, ec);
    fs::create_directories(g_sandbox, ec);
    int rc = Catch::Session().run(argc, argv);
    fs::remove_all(g_sandbox, ec);
    return rc;
}

// --- Settings ---------------------------------------------------------------

TEST_CASE("Default Settings carry the shipped defaults", "[config]") {
    Config s;
    CHECK(s.screenWidth == 1620);
    CHECK(s.screenHeight == 920);
    CHECK_FALSE(s.fullscreen);
    CHECK(s.targetFps == 60);
    CHECK(s.downloadImages);

    // Default-constructed paths point at the standard data/ layout.
    CHECK(s.paths.cardsJson.filename() == "cards.json");
    CHECK(s.paths.banlistJson.filename() == "banlist.json");
    CHECK(s.paths.cardImgDir.filename() == "images");
    CHECK(s.paths.cardBackImg.filename() == "card_back.png");
    // No provider endpoints are baked into the code — they live in the
    // content layer (data/settings.json, `url` group).
    CHECK(s.paths.cardImgUrl.empty());
    CHECK(s.paths.cardImgSmallUrl.empty());
    CHECK(s.paths.cardsJsonUrl.empty());
}

TEST_CASE("Settings Save/Load round-trips through user_settings.json", "[config]") {
    Config s;
    s.baseDir_       = g_sandbox / "bin" / "data";
    s.screenWidth    = 1280;
    s.screenHeight   = 720;
    s.fullscreen     = true;
    s.targetFps      = 144;
    s.downloadImages = false;
    s.paths.cardsJson = "custom_cards.json";
    REQUIRE(s.Save());
    REQUIRE(fs::exists(s.settingsFile()));

    // Load via a fake argv0 whose exe dir is the sandbox bin/ directory.
    Config loaded = Config::Load(fakeArgv0("bin").c_str());
    CHECK(loaded.screenWidth == 1280);
    CHECK(loaded.screenHeight == 720);
    CHECK(loaded.fullscreen);
    CHECK(loaded.targetFps == 144);
    CHECK_FALSE(loaded.downloadImages);
    CHECK(loaded.paths.cardsJson.filename() == "custom_cards.json");
    // Non-overridden paths are re-derived from the resolved base dir.
    CHECK(loaded.paths.banlistJson.filename() == "banlist.json");
}

TEST_CASE("Load with no file present returns defaults", "[config]") {
    Config loaded = Config::Load(fakeArgv0("empty").c_str());
    CHECK(loaded.screenWidth == 1620);
    CHECK(loaded.targetFps == 60);
    CHECK(loaded.paths.cardsJson.filename() == "cards.json");
}

TEST_CASE("Partial user_settings.json overrides only what it sets", "[config]") {
    fs::path dir = g_sandbox / "partial" / "data";
    fs::create_directories(dir);
    {
        std::FILE* f = std::fopen((dir / "user_settings.json").string().c_str(), "w");
        REQUIRE(f != nullptr);
        std::fputs(R"({"targetFps": 144, "paths": {"cardsJson": "mine.json"}})", f);
        std::fclose(f);
    }
    Config loaded = Config::Load(fakeArgv0("partial").c_str());
    CHECK(loaded.targetFps == 144);        // overridden
    CHECK(loaded.screenWidth == 1620);     // default preserved
    CHECK(loaded.paths.cardsJson.filename() == "mine.json"); // overridden
    CHECK(loaded.paths.cardImgDir.filename() == "images");   // default preserved
}

TEST_CASE("Nested settings.json schema (file/dir/url/app) is honored", "[config]") {
    fs::path dir = g_sandbox / "nested" / "data";
    fs::create_directories(dir);
    {
        std::FILE* f = std::fopen((dir / "settings.json").string().c_str(), "w");
        REQUIRE(f != nullptr);
        std::fputs(R"({"file": {"cardsJson": "data/cards.json", "banlistJson": "data/banlist.json",)"
                   R"("cardBackImg": "data/card_back.png"},)"
                   R"("dir": {"cardImgDir": "data/images/"},)"
                   R"("url": {"cardsJsonUrl": "https://cards.example.test/api/v7/cardinfo.php",)"
                   R"("cardImgSmallUrl": "https://images.example.test/cards_small/",)"
                   R"("cardImgUrl": "https://images.example.test/cards/"},)"
                   R"("app": {"screenWidth": 800, "screenHeight": 600, "fullscreen": true,)"
                   R"("targetFps": 30, "downloadImages": false}})", f);
        std::fclose(f);
    }
    Config loaded = Config::Load(fakeArgv0("nested").c_str());
    CHECK(loaded.screenWidth == 800);
    CHECK(loaded.screenHeight == 600);
    CHECK(loaded.fullscreen);
    CHECK(loaded.targetFps == 30);
    CHECK_FALSE(loaded.downloadImages);
    CHECK(loaded.paths.cardsJsonUrl == "https://cards.example.test/api/v7/cardinfo.php");
    CHECK(loaded.paths.cardImgUrl == "https://images.example.test/cards/");
    CHECK(loaded.paths.cardImgSmallUrl == "https://images.example.test/cards_small/");
    // Content-root-relative "data/..." values collapse onto the data dir.
    CHECK(loaded.paths.cardsJson == dir / "cards.json");
    CHECK(loaded.paths.banlistJson == dir / "banlist.json");
    CHECK(loaded.paths.cardImgDir == dir / "images");
    CHECK(loaded.paths.cardBackImg == dir / "card_back.png");
}

TEST_CASE("user_settings.json overrides the shipped settings.json", "[config]") {
    fs::path dir = g_sandbox / "override" / "data";
    fs::create_directories(dir);
    auto write = [&](const char* name, const char* body) {
        std::FILE* f = std::fopen((dir / name).string().c_str(), "w");
        REQUIRE(f != nullptr);
        std::fputs(body, f);
        std::fclose(f);
    };
    write("settings.json",
          R"({"app": {"targetFps": 30, "screenWidth": 800},)"
          R"("dir": {"cardImgDir": "data/images/"}})");
    write("user_settings.json", R"({"app": {"targetFps": 90}})");
    Config loaded = Config::Load(fakeArgv0("override").c_str());
    CHECK(loaded.targetFps == 90);                     // user file wins
    CHECK(loaded.screenWidth == 800);                  // reference value survives
    CHECK(loaded.paths.cardImgDir == dir / "images");  // reference path survives
}

TEST_CASE("exeDir/settingsFile static resolution", "[config]") {
    CHECK(Config::exeDir(nullptr) == fs::current_path());
    CHECK(Config::settingsFile(nullptr).filename() == "user_settings.json");
    CHECK(Config::settingsFile(nullptr).parent_path().filename() == "data");
    CHECK(Config::referenceFile(nullptr).filename() == "settings.json");
    CHECK(Config::referenceFile(nullptr).parent_path() ==
          Config::settingsFile(nullptr).parent_path());
}

// --- window fields (absorbed AppConfig; one source of truth) ------------------

TEST_CASE("Config carries window defaults incl. the title", "[config]") {
    Config c{};
    CHECK(c.screenWidth == 1620);
    CHECK(c.screenHeight == 920);
    CHECK(c.targetFps == 60);
    CHECK_FALSE(c.fullscreen);
    CHECK(std::string(c.windowTitle) == "OpenJoey");
}

// --- ActionId ----------------------------------------------------------------

TEST_CASE("ActionId catalog is appended-only (leading values pinned)", "[actionid]") {
    // The values below were assigned at extraction time and must never shift;
    // new entries may only be appended (see docs/API.md).
    STATIC_REQUIRE(static_cast<uint16_t>(openjoey::ActionId::None) == 0);
    STATIC_REQUIRE(static_cast<uint16_t>(openjoey::ActionId::Move_Draw) == 5);
    STATIC_REQUIRE(static_cast<uint16_t>(openjoey::ActionId::Summon_Normal) == 13);
}
