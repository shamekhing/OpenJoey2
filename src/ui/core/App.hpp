#pragma once
#include "ContentPaths.hpp"
#include "card/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/core/AppContext.hpp"
#include "ui/core/Event.hpp"
#include "ui/core/ScreenManager.hpp"
#include "ui/platform/AppConfig.hpp"
#include "ui/platform/PlatformContext.hpp"
#include "ui/screens/DeckEditorScreen.hpp"
#include "ui/screens/DuelScreen.hpp"
#include "ui/screens/MainMenuScreen.hpp"
#include <iostream>
#include <memory>
#include <raylib.h>
#include <vector>

namespace openjoey::ui {

class App {
public:
    App() : appConfig_{}, platform_(appConfig_),
            ctx_{cardDb_, selectedDeck_, imageCache_} {}
    ~App() = default;
    App(const App&) = delete;
    App& operator=(const App&) = delete;

    void Run();

private:
    void LoadCards();
    std::unique_ptr<IScreen> makeScreen(AppScreen s);
    void handleEvent(const ScreenEvent& ev);

    // Declaration order matters: ctx_ must come after the three fields it references.
    AppConfig                    appConfig_;
    PlatformContext              platform_;
    openjoey::CardDatabase       cardDb_;
    std::vector<openjoey::Card>  selectedDeck_;
    CardImageCache               imageCache_;
    AppContext                   ctx_;          // references cardDb_, selectedDeck_, imageCache_
    ScreenManager                screenManager_;
};

} // namespace openjoey::ui

inline void openjoey::ui::App::LoadCards() {
    const std::string path = openjoey::ContentPaths::cardsJson().string();
    if (!cardDb_.LoadFromFile(path))
        std::cerr << "[App] Failed to load " << path << "\n";
}

inline std::unique_ptr<openjoey::ui::IScreen>
openjoey::ui::App::makeScreen(AppScreen s) {
    switch (s) {
    case AppScreen::MainMenu:   return std::make_unique<MainMenuScreen>(ctx_);
    case AppScreen::DeckEditor: return std::make_unique<DeckEditorScreen>(ctx_);
    case AppScreen::Duel:       return std::make_unique<DuelScreen>(ctx_);
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
