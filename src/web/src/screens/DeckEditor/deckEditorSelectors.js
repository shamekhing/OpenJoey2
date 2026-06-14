(function () {
  const {
    SORT_LABELS,
    TYPE_LABELS,
    MIN_DECK_SIZE,
    MAX_DECK_SIZE,
  } = window.OpenJoeyDeckConstants;

  /**
   * Read-only derived values for the deck editor view.
   */
  function selectedCard(state, deck) {
    if (state.focusPool) return state.filtered[state.poolCursor] || null;
    return deck.cards[state.deckCursor] || null;
  }

  function poolBadge(state) {
    return `${SORT_LABELS[state.sortMode]} [${TYPE_LABELS[state.typeFilter]}] ${state.filtered.length} cards`;
  }

  function deckBadge(stats) {
    return stats.total < MIN_DECK_SIZE
      ? `${stats.total}/${MIN_DECK_SIZE} cards`
      : `${stats.total}/${MAX_DECK_SIZE} cards`;
  }

  function helpText(mode) {
    if (mode === "wide") {
      return "[TAB] switch [Arrows] navigate [ENTER] add [DEL/D] remove [O] sort [T] filter [G] grid/list [C] clear [S] save [L] load [F] duel [ESC] menu";
    }
    return "[TAB] switch [ENTER] add [DEL/D] remove [O/T] sort/filter [G] grid [F] duel [ESC] menu";
  }

  window.OpenJoeyDeckEditorSelectors = { selectedCard, poolBadge, deckBadge, helpText };
})();
