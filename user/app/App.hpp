#pragma once
#include "card/CardDatabase.hpp"
#include "card/ui/CardImageCache.hpp"
#include "ui/AppScreen.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/core/Event.hpp"
#include "ui/core/ScreenManager.hpp"
#include "ui/platform/AppConfig.hpp"
#include "ui/platform/PlatformContext.hpp"
#include "ui/platform/Settings.hpp"
#include "ui/screens/DeckEditorScreen.hpp"
#include "field/ui/DuelScreen.hpp"
#include "ui/screens/MainMenuScreen.hpp"
#include "ui/screens/SettingsScreen.hpp"
#include "ui/screens/TestingScreen.hpp"
#include <iostream>
#include <memory>
#include <raylib.h>
#include <vector>

namespace openjoey::ui {

class App {
public:
        App() : settings_(Settings::Load()),
            appConfig_(makeConfig(settings_)),
            platform_(appConfig_),
            imageCache_(settings_.paths.cardImgDir,
                        settings_.paths.ygoprodeckUrl,
                        settings_.paths.ygoprodeckUrlSmall),
            ctx_{cardDb_, selectedDeck_, imageCache_, settings_} {}
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Run();

private:
    static AppConfig           makeConfig(const Settings& s);
    void LoadCards();
    std::unique_ptr<IScreen> makeScreen(AppScreen s);
    void handleEvent(const ScreenEvent& ev);

    // Declaration order matters: ctx_ must come after the fields it references
    // (settings_ must precede appConfig_ and ctx_).
    Settings                     settings_;
    AppConfig                    appConfig_;
    PlatformContext              platform_;
    openjoey::CardDatabase       cardDb_;
    std::vector<openjoey::Card>  selectedDeck_;
    CardImageCache               imageCache_;
    AppContext                   ctx_;          // references the above + settings_
    ScreenManager                screenManager_;
};

} // namespace openjoey::ui

inline openjoey::ui::AppConfig
openjoey::ui::App::makeConfig(const Settings& s) {
    AppConfig cfg;
    cfg.screenWidth  = s.screenWidth;
    cfg.screenHeight = s.screenHeight;
    cfg.targetFps    = s.targetFps;
    cfg.fullscreen   = s.fullscreen;
    return cfg;
}

inline void openjoey::ui::App::LoadCards() {
        const std::string path = settings_.paths.cardsJson.string();
    if (!cardDb_.LoadFromFile(path))
        std::cerr << "[App] Failed to load " << path << "\n";
}

inline std::unique_ptr<openjoey::ui::IScreen>
openjoey::ui::App::makeScreen(AppScreen s) {
    switch (s) {
    case AppScreen::MainMenu:   return std::make_unique<MainMenuScreen>(ctx_);
    case AppScreen::DeckEditor: return std::make_unique<DeckEditorScreen>(ctx_);
    case AppScreen::Duel:       return std::make_unique<DuelScreen>(ctx_);
    case AppScreen::Testing:    return std::make_unique<TestingScreen>(ctx_);
    case AppScreen::Settings:   return std::make_unique<SettingsScreen>(ctx_);
    default:                    return std::make_unique<MainMenuScreen>(ctx_);
    }
}

inline void openjoey::ui::App::handleEvent(const ScreenEvent& ev) {
    if (ev.type == ScreenEvent::Type::Replace)
        screenManager_.Replace(makeScreen(ev.target));
}

inline void openjoey::ui::App::Run() {
    LoadCards();
    screenManager_.Replace(makeScreen(AppScreen::MainMenu));

    while (!WindowShouldClose() && !screenManager_.Empty()) {
        float dt = GetFrameTime();
        imageCache_.PollAndLoad();

        ScreenEvent ev = screenManager_.Top().Update(dt);
        if (ev.type == ScreenEvent::Type::Quit) break;
        handleEvent(ev);

        BeginDrawing();
        if (!screenManager_.Empty())
            screenManager_.Top().Draw();
        EndDrawing();
    }
}
