(function () {
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
  const DECK_STORAGE_KEY = "openjoey2.src2.deck";

  window.OpenJoeyDeckConstants = {
    SORT_LABELS,
    TYPE_LABELS,
    MIN_DECK_SIZE,
    MAX_DECK_SIZE,
    MAX_COPIES,
    CARD_ASPECT,
    DECK_STORAGE_KEY,
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
    state.filtered = cardDb.filter(state.query, state.typeFilter);
    state.poolCursor = Math.max(0, Math.min(state.poolCursor, state.filtered.length - 1));
  }

  function clamp(state, deck) {
    state.poolCursor = Math.max(0, Math.min(state.poolCursor, state.filtered.length - 1));
    state.deckCursor = Math.max(0, Math.min(state.deckCursor, deck.cards.length - 1));
  }

  window.OpenJoeyDeckEditorState = { createState, rebuild, clamp };
})();
