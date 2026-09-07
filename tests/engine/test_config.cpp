#include "support/Fixtures.hpp"

// ── from tests.cpp lines 223,244 ──
TEST_CASE("Config round-trips user_settings.json", "[config]") {
    Config s;
    s.screenWidth = 1366;
    s.screenHeight = 768;
    s.targetFps = 120;
    s.fullscreen = true;
    s.downloadImages = false;
    REQUIRE(s.Save());

    Config loaded = Config::Load();
    REQUIRE(loaded.screenWidth == 1366);
    REQUIRE(loaded.screenHeight == 768);
    REQUIRE(loaded.targetFps == 120);
    REQUIRE(loaded.fullscreen == true);
    REQUIRE(loaded.downloadImages == false);

        std::error_code ec;
    std::filesystem::remove(Config::settingsFile(nullptr), ec);
}

// ── Card effect subscription (layer 1 carries the data; no later-layer dep) ──


