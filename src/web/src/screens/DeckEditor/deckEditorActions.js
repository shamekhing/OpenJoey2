(function () {
  const State = window.OpenJoeyDeckEditorState;
  const Layout = window.OpenJoeyDeckEditorLayout;
  const Storage = window.OpenJoeyDeckStorage;
  const { SORT_LABELS, TYPE_LABELS, MIN_DECK_SIZE, MAX_DECK_SIZE } = window.OpenJoeyDeckConstants;

  const BLOCKED_KEYS = [
    "Tab",
    "ArrowDown",
    "ArrowUp",
    "ArrowLeft",
    "ArrowRight",
    "PageDown",
    "PageUp",
    "Enter",
    "Delete",
    "Backspace",
  ];

  function search(app, state, query) {
    state.query = query;
    state.poolCursor = 0;
    State.rebuild(state, app.cardDb);
    State.clamp(state, app.deck);
  }

  function key(app, state, layout, event) {
    if (event.target === app.searchInput && event.key !== "Enter" && event.key !== "Escape") return;
    if (BLOCKED_KEYS.includes(event.key)) event.preventDefault();
    const step = !state.focusPool && state.deckGrid ? Layout.gridCols(layout.deck) : 1;

    if (event.key === "Escape") app.goto("menu");
    else if (event.key === "Tab") state.focusPool = !state.focusPool;
    else if (event.key === "ArrowDown") move(state, step);
    else if (event.key === "ArrowUp") move(state, -step);
    else if (event.key === "PageDown") move(state, step * 8);
    else if (event.key === "PageUp") move(state, -step * 8);
    else if (event.key === "ArrowRight") state.focusPool ? (state.focusPool = false) : move(state, 1);
    else if (event.key === "ArrowLeft") !state.focusPool && state.deckGrid ? move(state, -1) : (state.focusPool = true);
    else if (event.key === "Enter") add(app, state);
    else if (event.key === "Delete" || event.key === "Backspace" || event.key.toLowerCase() === "d") remove(app, state);
    else if (event.key.toLowerCase() === "o") sort(app, state);
    else if (event.key.toLowerCase() === "t") type(app, state);
    else if (event.key.toLowerCase() === "g") state.deckGrid = !state.deckGrid;
    else if (event.key.toLowerCase() === "c") clear(app, state);
    else if (event.key.toLowerCase() === "s") save(app);
    else if (event.key.toLowerCase() === "l") load(app, state);
    else if (event.key.toLowerCase() === "f") duel(app);

    State.clamp(state, app.deck);
  }

  function click(app, state, layout, x, y) {
    const pool = Layout.hitList(layout.pool, state.filtered, state.poolCursor, 108, false, x, y);
    if (pool >= 0) {
      state.focusPool = true;
      state.poolCursor = pool;
      return;
    }
    const deck = Layout.hitList(layout.deck, app.deck.cards, state.deckCursor, 118, state.deckGrid, x, y);
    if (deck >= 0) {
      state.focusPool = false;
      state.deckCursor = deck;
    }
  }

  function doubleClick(app, state) {
    if (state.focusPool) add(app, state);
    else remove(app, state);
  }

  function move(state, delta) {
    if (state.focusPool) state.poolCursor += delta;
    else state.deckCursor += delta;
  }

  function add(app, state) {
    const card = state.filtered[state.poolCursor];
    if (!card) return;
    if (app.deck.add(card)) {
      state.deckCursor = app.deck.cards.length - 1;
      app.status = `Added: ${card.name}`;
      Storage.save(app.deck);
      return;
    }
    app.status = app.deck.countCopies(card.id) >= 3
      ? `Max 3 copies of ${card.name}`
      : `Deck full (${MAX_DECK_SIZE} cards max)`;
  }

  function remove(app, state) {
    if (!app.deck.cards.length) return;
    const card = app.deck.cards[state.deckCursor];
    if (app.deck.removeAt(state.deckCursor)) {
      app.status = `Removed: ${card.name}`;
      Storage.save(app.deck);
    }
  }

  function sort(app, state) {
    state.sortMode = (state.sortMode + 1) % SORT_LABELS.length;
    app.cardDb.rebuildSort(state.sortMode);
    state.poolCursor = 0;
    State.rebuild(state, app.cardDb);
    app.status = `Sort: ${SORT_LABELS[state.sortMode]}`;
  }

  function type(app, state) {
    state.typeFilter = (state.typeFilter + 1) % TYPE_LABELS.length;
    state.poolCursor = 0;
    State.rebuild(state, app.cardDb);
    app.status = `Filter: ${TYPE_LABELS[state.typeFilter]}`;
  }

  function clear(app, state) {
    app.deck.clear();
    state.deckCursor = 0;
    Storage.save(app.deck);
    app.status = "Deck cleared";
  }

  function load(app, state) {
    const count = Storage.load(app.deck, app.cardDb);
    state.deckCursor = 0;
    app.status = count ? "Loaded deck" : "No saved deck";
  }

  function save(app) {
    Storage.save(app.deck);
    app.status = "Saved deck";
  }

  function duel(app) {
    const need = MIN_DECK_SIZE - app.deck.cards.length;
    if (!app.deck.canDuel()) {
      app.status = `Need ${need} more cards`;
      return;
    }
    app.goto("duel");
  }

  window.OpenJoeyDeckEditorActions = { search, key, click, doubleClick };
})();
