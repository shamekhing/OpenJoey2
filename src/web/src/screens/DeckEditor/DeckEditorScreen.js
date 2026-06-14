(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Actions = window.OpenJoeyDeckEditorActions;
  const Layout = window.OpenJoeyDeckEditorLayout;
  const State = window.OpenJoeyDeckEditorState;
  const View = window.OpenJoeyDeckEditorView;

  /**
   * Deck editor screen lifecycle.
   *
   * Keeps DOM search input ownership here while actions/state/view stay
   * canvas-oriented and dependency-light.
   */
  class DeckEditorScreen {
    constructor(app) {
      this.app = app;
      this.state = State.createState();
      this.app.cardDb.rebuildSort(this.state.sortMode);
      State.rebuild(this.state, this.app.cardDb);
      State.clamp(this.state, this.app.deck);
      this.app.searchInput.value = "";
      this.app.searchInput.style.display = "block";
      this.inputHandler = () => {
        Actions.search(this.app, this.state, this.app.searchInput.value);
      };
      this.app.searchInput.addEventListener("input", this.inputHandler);
    }

    layout() {
      this.layoutData = Layout.compute(this.app);
      // The search box is real DOM for IME/accessibility; align it to canvas layout.
      this.app.searchInput.style.left = `${this.layoutData.search.left}px`;
      this.app.searchInput.style.top = `${this.layoutData.search.top}px`;
      this.app.searchInput.style.width = `${this.layoutData.search.width}px`;
    }

    dispose() {
      // Route changes must remove DOM listeners because screens are recreated.
      this.app.searchInput.removeEventListener("input", this.inputHandler);
    }

    key(event) {
      Actions.key(this.app, this.state, this.layoutData, event);
    }

    click(x, y) {
      Actions.click(this.app, this.state, this.layoutData, x, y);
    }

    doubleClick() {
      Actions.doubleClick(this.app, this.state);
    }

    draw(g) {
      View.draw(g, this.app, this.state, this.layoutData);
    }
  }

  window.OpenJoeyScreens.DeckEditorScreen = DeckEditorScreen;
})();
