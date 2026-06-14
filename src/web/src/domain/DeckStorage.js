(function () {
  const { DECK_STORAGE_KEY, DECK_FOLDER_STORAGE_KEY } = window.OpenJoeyDeckConstants;

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

  function parseDeckText(text) {
    return String(text || "")
      .split(/\r?\n/)
      .map((line) => line.trim())
      .filter((line) => line && !line.startsWith("#") && !line.startsWith("!"))
      .map((line) => Number(line))
      .filter((id) => Number.isInteger(id) && id > 0);
  }

  function saveDeckFolder(decks) {
    localStorage.setItem(DECK_FOLDER_STORAGE_KEY, JSON.stringify(decks));
  }

  function loadDeckFolder() {
    return JSON.parse(localStorage.getItem(DECK_FOLDER_STORAGE_KEY) || "[]");
  }

  async function importDeckFolder(files, cardDb) {
    const decks = [];
    for (const file of files) {
      const text = await file.text();
      const ids = parseDeckText(text).filter((id) => cardDb.byId.has(id));
      if (ids.length) decks.push({ name: file.webkitRelativePath || file.name, ids });
    }
    saveDeckFolder(decks);
    return decks;
  }

  function loadIds(deck, cardDb, ids) {
    deck.clear();
    for (const id of ids) {
      const card = cardDb.byId.get(id);
      if (card) deck.add(card);
    }
    save(deck);
    return deck.cards.length;
  }

  window.OpenJoeyDeckStorage = {
    deckIds,
    save,
    load,
    exportText,
    parseDeckText,
    importDeckFolder,
    loadDeckFolder,
    loadIds,
  };
})();
