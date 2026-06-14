(function () {
  const { DECK_STORAGE_KEY } = window.OpenJoeyDeckConstants;

  /**
   * Persist decks by passcode only; card metadata is rehydrated from CardDb.
   */
  function deckIds(deck) {
    return deck.cards.map((card) => card.id);
  }

  function save(deck) {
    localStorage.setItem(DECK_STORAGE_KEY, JSON.stringify(deckIds(deck)));
  }

  function load(deck, cardDb) {
    const ids = JSON.parse(localStorage.getItem(DECK_STORAGE_KEY) || "[]");
    deck.clear();
    for (const id of ids) {
      const card = cardDb.byId.get(id);
      if (card) deck.add(card);
    }
    return deck.cards.length;
  }

  function exportText(deck) {
    return deckIds(deck).join("\n");
  }

  window.OpenJoeyDeckStorage = { deckIds, save, load, exportText };
})();
