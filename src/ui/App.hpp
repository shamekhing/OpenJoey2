#pragma once
#include "ContentPaths.hpp"
#include "card/CardDatabase.hpp"
#include "ui/AppScreen.hpp"
#include "ui/platform/AppConfig.hpp"
#include "ui/platform/PlatformContext.hpp"
#include "ui/screens/DeckEditorScreen.hpp"
#include "ui/screens/MainMenuScreen.hpp"
#include <iostream>
#include <raylib.h>
#include <vector>

namespace openjoey::ui {

class App {
public:
  App() : appConfig_{}, platform_(appConfig_) {};
  ~App() {};
  void Run();

private:
  void LoadCards() {
    const std::string path = openjoey::ContentPaths::cardsJson().string();
    if (!cardDb_.LoadFromFile(path))
      std::cerr << "[App] Failed to load " << path << "\n";
  }
  /*void StartDuel(const std::vector<openjoey::Card> &deck) {
    duel_.beginMirrorMatch(deck);
  }*/

  AppConfig appConfig_;
  PlatformContext platform_;
  AppScreen currentScreen_ = AppScreen::DeckEditor;
  openjoey::CardDatabase cardDb_;
  std::vector<openjoey::Card> selectedDeck_;
};

} // namespace openjoey::ui

void openjoey::ui::App::Run() {
  LoadCards();
  while (true) {
    switch (currentScreen_) {

    case AppScreen::MainMenu: {
      MainMenuScreen menu;
      while (!WindowShouldClose()) {
        menu.Update();
        if (menu.ShouldQuit())
          return;
        AppScreen next = menu.NextScreen();
        if (next != AppScreen::MainMenu) {
          currentScreen_ = next;
          break;
        }
        BeginDrawing();
        menu.Draw();
        EndDrawing();
      }
      if (WindowShouldClose())
        return;
      break;
    }

    case AppScreen::DeckEditor: {
      DeckEditorScreen editor(cardDb_);
      while (!WindowShouldClose()) {
        editor.Update();
        AppScreen next = editor.NextScreen();
        if (next != AppScreen::DeckEditor) {
          if (editor.DeckReady())
            selectedDeck_ = editor.GetDeck();
          currentScreen_ = next;
          break;
        }
        BeginDrawing();
        editor.Draw();
        EndDrawing();
      }
      if (WindowShouldClose())
        return;
      break;
    }
      /*
              case AppScreen::Duel: {
                StartDuel(selectedDeck);
                DuelScreen duelScreen(platform_, duel_, cardDb_);
                duelScreen.Run();
                currentScreen_ = AppScreen::MainMenu;
                break;
              }

              case AppScreen::Settings: {
                currentScreen_ = AppScreen::MainMenu;
                break;
              }*/
    }
  }
}