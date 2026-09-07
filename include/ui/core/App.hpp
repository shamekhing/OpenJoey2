#pragma once
#include <raylib.h>

#include <iostream>
#include <memory>
#include <ui/cards/CardImageCache.hpp>
#include <vector>

#include "Config.hpp"
#include "cards/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/core/Event.hpp"
#include "ui/core/ScreenManager.hpp"
#include "ui/platform/PlatformContext.hpp"
#include "ui/screens/DeckEditorScreen.hpp"
#include "ui/screens/DuelScreen.hpp"
#include "ui/screens/MainMenuScreen.hpp"
#include "ui/screens/SettingsScreen.hpp"
#include "ui/screens/TestingScreen.hpp"

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

namespace openjoey::ui {
using cards::Card;
using cards::CardDatabase;

class App {
   public:
    explicit App(const char* argv0 = nullptr)
        : settings_(Config::Load(argv0)),
          platform_(settings_),
          imageCache_(settings_.paths.cardImgDir, settings_.paths.cardImgUrl,
                      settings_.paths.cardImgSmallUrl,
                      settings_.downloadImages),
          ctx_{cardDb_, selectedDeck_, imageCache_, settings_} {}
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Run();
    void tick();  // one frame (native loop body / web callback)

   private:
    static Config makeConfig(const Config& s);
    void LoadCards();
    std::unique_ptr<IScreen> makeScreen(AppScreen s);
    void handleEvent(const ScreenEvent& ev);

    // Declaration order matters: ctx_ must come after the fields it references
    // (settings_ must precede appConfig_ and ctx_).
    Config settings_;
    PlatformContext platform_;
    openjoey::cards::CardDatabase cardDb_;
    std::vector<openjoey::cards::Card> selectedDeck_;
    CardImageCache imageCache_;
    AppContext ctx_;  // references the above + settings_
    ScreenManager screenManager_;
};

}  // namespace openjoey::ui

inline openjoey::Config
openjoey::ui::App::makeConfig(const Config& s) {
    Config cfg;
    cfg.screenWidth = s.screenWidth;
    cfg.screenHeight = s.screenHeight;
    cfg.targetFps = s.targetFps;
    cfg.fullscreen = s.fullscreen;
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
        case AppScreen::MainMenu:
            return std::make_unique<MainMenuScreen>(ctx_);
        case AppScreen::DeckEditor:
            return std::make_unique<DeckEditorScreen>(ctx_);
        case AppScreen::Duel:
            return std::make_unique<DuelScreen>(ctx_);
        case AppScreen::Testing:
            return std::make_unique<TestingScreen>(ctx_);
        case AppScreen::Settings:
            return std::make_unique<SettingsScreen>(ctx_);
        default:
            return std::make_unique<MainMenuScreen>(ctx_);
    }
}

inline void openjoey::ui::App::handleEvent(const ScreenEvent& ev) {
    if (ev.type == ScreenEvent::Type::Replace)
        screenManager_.Replace(makeScreen(ev.target));
}

inline void openjoey::ui::App::Run() {
    LoadCards();
    screenManager_.Replace(makeScreen(AppScreen::MainMenu));

#ifdef __EMSCRIPTEN__
    // Web: the browser owns the loop — one frame per callback, never returns.
    auto tick = +[](void* self) {
        static_cast<App*>(self)->tick();
    };
    emscripten_set_main_loop_arg(tick, this, 0, 1);
#else
    while (!WindowShouldClose() && !screenManager_.Empty())
        tick();
#endif
}

// One frame. Native: called from Run()'s loop; web: the emscripten callback.
inline void openjoey::ui::App::tick() {
    float dt = GetFrameTime();
    imageCache_.PollAndLoad();

    ScreenEvent ev = screenManager_.Top().Update(dt);
    if (ev.type == ScreenEvent::Type::Quit) {
#ifndef __EMSCRIPTEN__
        return;  // native: loop exits on the WindowShouldClose check
#else
        emscripten_cancel_main_loop();
        return;
#endif
    }
    handleEvent(ev);

    BeginDrawing();
    if (!screenManager_.Empty())
        screenManager_.Top().Draw();
    EndDrawing();
}
