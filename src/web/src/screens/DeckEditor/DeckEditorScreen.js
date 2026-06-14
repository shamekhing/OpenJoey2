(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Actions = window.OpenJoeyDeckEditorActions;
  const Layout = window.OpenJoeyDeckEditorLayout;
  const State = window.OpenJoeyDeckEditorState;
  const View = window.OpenJoeyDeckEditorView;

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
      this.app.searchInput.style.left = `${this.layoutData.search.left}px`;
      this.app.searchInput.style.top = `${this.layoutData.search.top}px`;
      this.app.searchInput.style.width = `${this.layoutData.search.width}px`;
    }

    dispose() {
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
