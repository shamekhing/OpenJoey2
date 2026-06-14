/**
 * Browser bootstrap.
 *
 * Card rows are loaded from the bundled rows, IndexedDB cache, or the default
 * YGOProDeck URL. DeckBridge then chooses WASM when available.
 */
(async function () {
  async function loadInitialRows() {
    const {
      CARD_DB_MODE_STORAGE_KEY,
      CARD_DB_URL_STORAGE_KEY,
    } = window.OpenJoeyDeckConstants;
    const mode = localStorage.getItem(CARD_DB_MODE_STORAGE_KEY) || "auto";
    const bundled = window.OPENJOEY_CARD_ROWS || [];

    if ((mode === "auto" || mode === "bundled") && bundled.length) {
      return { rows: bundled, source: "bundled card DB" };
    }

    if (mode === "manual") return { rows: [], source: "manual card DB mode" };

    if (mode === "auto" || mode === "cache") {
      const cached = await window.OpenJoeyCardDbCache?.loadRows?.();
      if (cached?.length) return { rows: cached, source: "cached card DB" };
      if (mode === "cache") return { rows: [], source: "empty cache" };
    }

    if (mode === "auto" || mode === "remote") {
      const url = localStorage.getItem(CARD_DB_URL_STORAGE_KEY) || window.OPENJOEY_DEFAULT_CARD_DB_URL;
      try {
        const response = await fetch(url);
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const rows = window.OpenJoeyCardDb.rowsFromYgoProDeckJson(await response.json());
        if (rows.length) await window.OpenJoeyCardDbCache?.saveRows?.(rows);
        return { rows, source: "remote card DB" };
      } catch (error) {
        console.warn("Card DB unavailable; use Settings to load one", error);
      }
    }

    return { rows: [], source: "no card DB loaded" };
  }

  const { rows, source } = await loadInitialRows();
  const cardDb = new window.OpenJoeyCardDb.CardDb(rows);
  const { core, duel, backend } = await window.OpenJoeyDeckBridge.createDeckCore(cardDb);
  window.openJoeyApp = new window.OpenJoeyApp.App(
    document.getElementById("app"),
    document.getElementById("search"),
    cardDb,
    core,
    duel,
    backend,
  );
  window.openJoeyApp.status = `Ready (${backend}, ${source})`;
})();
