(function () {
  /**
   * Deck editor constants shared by actions, layout, storage, and views.
   */
  const SORT_LABELS = [
    "Type",
    "Name (A-Z)",
    "Name (Z-A)",
    "Level (desc)",
    "Level (asc)",
    "ATK (desc)",
    "ATK (asc)",
    "DEF (desc)",
    "DEF (asc)",
    "ID",
  ];

  const TYPE_LABELS = ["All", "Monster", "Spell", "Trap"];
  const MIN_DECK_SIZE = 40;
  const MAX_DECK_SIZE = 60;
  const MAX_COPIES = 3;
  const CARD_ASPECT = 86 / 59;
  const DECK_STORAGE_KEY = "openjoey2.web.deck";
  const DECK_FOLDER_STORAGE_KEY = "openjoey2.web.deckFolder";
  const CARD_DB_URL_STORAGE_KEY = "openjoey2.web.cardDbUrl";

  window.OpenJoeyDeckConstants = {
    SORT_LABELS,
    TYPE_LABELS,
    MIN_DECK_SIZE,
    MAX_DECK_SIZE,
    MAX_COPIES,
    CARD_ASPECT,
    DECK_STORAGE_KEY,
    DECK_FOLDER_STORAGE_KEY,
    CARD_DB_URL_STORAGE_KEY,
  };

  function createState() {
    return {
      poolCursor: 0,
      deckCursor: 0,
      focusPool: true,
      deckGrid: false,
      sortMode: 0,
      typeFilter: 0,
      query: "",
      filtered: [],
    };
  }

  function rebuild(state, cardDb) {
    // Filtering is intentionally derived state: rebuild after query/sort/type changes.
    state.filtered = cardDb.filter(state.query, state.typeFilter);
    state.poolCursor = Math.max(0, Math.min(state.poolCursor, state.filtered.length - 1));
  }

  function clamp(state, deck) {
    // Keep cursors valid after filtering, adding, removing, or loading decks.
    state.poolCursor = Math.max(0, Math.min(state.poolCursor, state.filtered.length - 1));
    state.deckCursor = Math.max(0, Math.min(state.deckCursor, deck.cards.length - 1));
  }

  window.OpenJoeyDeckEditorState = { createState, rebuild, clamp };
})();
