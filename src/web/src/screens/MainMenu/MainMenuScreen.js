(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Actions = window.OpenJoeyMainMenuActions;
  const View = window.OpenJoeyMainMenuView;

  /**
   * Main menu screen shell. Delegates input mutations to Actions and drawing to View.
   */
  class MainMenuScreen {
    constructor(app) {
      this.app = app;
      this.items = [
        ["Duel", "duel"],
        ["Deck Editor", "deck"],
        ["Settings", "settings"],
        ["Testing", "testing"],
        ["Quit", "quit"],
      ];
      this.state = { cursor: 0 };
    }

    key(event) {
      Actions.key(this.app, this.state, this.items, event);
    }

    click(x, y) {
      Actions.click(this.app, this.state, this.items, View.metrics(this.app, this.items.length), x, y);
    }

    draw(g) {
      View.draw(g, this.app, this.state, this.items);
    }
  }

  window.OpenJoeyScreens.MainMenuScreen = MainMenuScreen;
})();
