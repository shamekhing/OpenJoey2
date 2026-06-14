(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const CardDb = window.OpenJoeyCardDb;
  const Storage = window.OpenJoeyDeckStorage;
  const {
    CARD_DB_MODE_STORAGE_KEY,
    CARD_DB_FILTER_STORAGE_KEY,
    CARD_DB_URL_STORAGE_KEY,
  } = window.OpenJoeyDeckConstants;

  const DEFAULT_CARD_DB_URL = "https://db.ygoprodeck.com/api/v7/cardinfo.php";

  class SettingsScreen {
    constructor(app) {
      this.app = app;
      this.root = document.createElement("section");
      this.root.className = "settings-panel";
      const savedUrl = localStorage.getItem(CARD_DB_URL_STORAGE_KEY) || DEFAULT_CARD_DB_URL;
      const savedMode = localStorage.getItem(CARD_DB_MODE_STORAGE_KEY) || "auto";
      const savedFilter = localStorage.getItem(CARD_DB_FILTER_STORAGE_KEY) || window.OpenJoeyCardDb.CARD_DB_FILTER_ALL;
      this.root.innerHTML = `
        <div id="card-db-drop" class="settings-drop" tabindex="0">
          <div>
            <strong>Card DB</strong>
            <span>Drop cardinfo JSON, generated rows, or a .db text file</span>
          </div>
          <button id="card-db-pick" type="button">Choose file</button>
          <input id="card-db-file" type="file" accept=".json,.js,.txt,.db,application/json">
        </div>
        <div class="settings-row settings-url-row">
          <label for="card-db-url">Card DB URL</label>
          <input id="card-db-url" type="url" value="${savedUrl}" spellcheck="false">
          <button id="card-db-load" type="button">Load</button>
        </div>
        <div class="settings-row settings-db-row">
          <label for="card-db-mode">Card DB startup</label>
          <select id="card-db-mode">
            <option value="auto">Auto: bundled, cache, then URL</option>
            <option value="cache">Cache only</option>
            <option value="remote">URL every startup</option>
            <option value="manual">Manual upload only</option>
          </select>
          <button id="card-db-refresh" type="button">Refresh URL</button>
          <button id="card-db-clear" type="button">Clear cache</button>
        </div>
        <div class="settings-row settings-db-row">
          <label for="card-db-filter">Card DB era</label>
          <select id="card-db-filter">
            <option value="all">All cards</option>
            <option value="gx">GX and earlier</option>
          </select>
        </div>
        <div id="deck-folder-drop" class="settings-drop" tabindex="0">
          <div>
            <strong>Deck folder</strong>
            <span>Drop a folder or deck files with passcode lists</span>
          </div>
          <button id="deck-folder-pick" type="button">Choose folder</button>
          <input id="deck-folder" type="file" webkitdirectory directory multiple>
        </div>
      `;
      document.body.appendChild(this.root);
      this.cardDrop = this.root.querySelector("#card-db-drop");
      this.cardPick = this.root.querySelector("#card-db-pick");
      this.cardFile = this.root.querySelector("#card-db-file");
      this.cardUrl = this.root.querySelector("#card-db-url");
      this.loadUrlButton = this.root.querySelector("#card-db-load");
      this.cardMode = this.root.querySelector("#card-db-mode");
      this.cardMode.value = savedMode;
      this.cardFilter = this.root.querySelector("#card-db-filter");
      this.cardFilter.value = savedFilter;
      if (!this.cardFilter.value) {
        this.cardFilter.value = window.OpenJoeyCardDb.CARD_DB_FILTER_ALL;
        localStorage.setItem(CARD_DB_FILTER_STORAGE_KEY, this.cardFilter.value);
      }
      this.refreshUrlButton = this.root.querySelector("#card-db-refresh");
      this.clearCacheButton = this.root.querySelector("#card-db-clear");
      this.deckDrop = this.root.querySelector("#deck-folder-drop");
      this.deckPick = this.root.querySelector("#deck-folder-pick");
      this.deckFolder = this.root.querySelector("#deck-folder");
      this.handlers = [
        [this.cardPick, "click", () => this.cardFile.click()],
        [this.deckPick, "click", () => this.deckFolder.click()],
        [this.cardFile, "change", () => this.loadCardFiles([...this.cardFile.files])],
        [this.loadUrlButton, "click", () => this.loadCardUrl()],
        [this.cardMode, "change", () => this.saveCardMode()],
        [this.cardFilter, "change", () => this.saveCardFilter()],
        [this.refreshUrlButton, "click", () => this.loadCardUrl(true)],
        [this.clearCacheButton, "click", () => this.clearCardCache()],
        [this.deckFolder, "change", () => this.loadDeckFiles([...this.deckFolder.files])],
      ];
      for (const [node, event, handler] of this.handlers) node.addEventListener(event, handler);
      this.bindDrop(this.cardDrop, (files) => this.loadCardFiles(files));
      this.bindDrop(this.deckDrop, (files) => this.loadDeckFiles(files));
      this.layout();
    }

    layout() {
      const wide = this.app.w >= 760;
      this.root.style.left = `${wide ? 48 : 16}px`;
      this.root.style.right = `${wide ? "auto" : "16px"}`;
      this.root.style.top = `${this.app.chromeTop() + 72}px`;
      this.root.style.width = wide ? "640px" : "auto";
    }

    dispose() {
      for (const [node, event, handler] of this.handlers) node.removeEventListener(event, handler);
      this.root.remove();
    }

    key(event) {
      if (event.key === "Escape") {
        event.preventDefault();
        this.app.goto("menu");
      }
    }

    bindDrop(node, onFiles) {
      node.addEventListener("dragover", (event) => {
        event.preventDefault();
        node.classList.add("is-dragging");
      });
      node.addEventListener("dragleave", () => node.classList.remove("is-dragging"));
      node.addEventListener("drop", async (event) => {
        event.preventDefault();
        node.classList.remove("is-dragging");
        const files = await filesFromDrop(event.dataTransfer);
        onFiles(files);
      });
    }

    async loadCardFiles(files) {
      const file = files[0];
      if (!file) return;
      try {
        const rows = CardDb.rowsFromText(await file.text());
        if (!rows.length) throw new Error("No cards found");
        this.app.replaceCardDb(rows, `Loaded ${file.name}`);
      } catch (error) {
        this.app.status = `Card DB load failed: ${error.message}`;
      }
    }

    async loadCardUrl() {
      try {
        const response = await fetch(this.cardUrl.value.trim());
        if (!response.ok) throw new Error(`HTTP ${response.status}`);
        const rows = CardDb.rowsFromYgoProDeckJson(await response.json());
        if (!rows.length) throw new Error("No cards found");
        localStorage.setItem(CARD_DB_URL_STORAGE_KEY, this.cardUrl.value.trim());
        localStorage.setItem(CARD_DB_MODE_STORAGE_KEY, "cache");
        this.cardMode.value = "cache";
        this.app.replaceCardDb(rows, "Loaded card DB URL");
      } catch (error) {
        this.app.status = `Card DB URL failed: ${error.message}`;
      }
    }

    saveCardMode() {
      localStorage.setItem(CARD_DB_MODE_STORAGE_KEY, this.cardMode.value);
      this.app.status = `Card DB startup: ${this.cardMode.options[this.cardMode.selectedIndex].text}`;
    }

    saveCardFilter() {
      localStorage.setItem(CARD_DB_FILTER_STORAGE_KEY, this.cardFilter.value);
      this.app.applyCardDbFilter(`Card DB filter: ${this.cardFilter.options[this.cardFilter.selectedIndex].text}`);
    }

    async clearCardCache() {
      await window.OpenJoeyCardDbCache?.clearRows?.();
      if (this.cardMode.value === "cache") {
        this.app.replaceCardDb([], "Cleared card DB cache");
      } else {
        this.app.status = "Card DB cache cleared";
      }
    }

    async loadDeckFiles(files) {
      if (!files.length) return;
      try {
        const decks = await Storage.importDeckFolder(files, this.app.cardDb);
        const first = decks[0];
        const loaded = first ? Storage.loadIds(this.app.deck, this.app.cardDb, first.ids) : 0;
        this.app.status = first
          ? `Imported ${decks.length} decks, loaded ${loaded} cards from ${first.name}`
          : "No valid deck files found";
      } catch (error) {
        this.app.status = `Deck folder import failed: ${error.message}`;
      }
    }

    draw(g) {
      this.app.drawChrome("SETTINGS", "Upload card DB JSON/generated rows, load YGOProDeck URL, choose era filtering, or import a deck folder. ESC back");
      const x = this.app.w < 520 ? 20 : 48;
      const y = this.app.chromeTop() + 42;
      g.fillStyle = "#f1f5f8";
      g.font = this.app.w < 520 ? "700 22px system-ui" : "700 28px system-ui";
      g.fillText("Settings", x, y, this.app.w - x * 2);
      g.fillStyle = "#9faab7";
      g.font = "14px system-ui";
      g.fillText(`Active card DB: ${this.app.cardDb.cards.length} cards`, x, y + 26, this.app.w - x * 2);
      g.fillText(`Startup source: ${this.cardMode?.value || "auto"}`, x, y + 46, this.app.w - x * 2);
    }
  }

  async function filesFromDrop(dataTransfer) {
    const items = [...(dataTransfer.items || [])];
    if (!items.length) return [...(dataTransfer.files || [])];
    const files = [];
    for (const item of items) {
      const entry = item.webkitGetAsEntry?.();
      if (entry) files.push(...await filesFromEntry(entry));
      else {
        const file = item.getAsFile?.();
        if (file) files.push(file);
      }
    }
    return files;
  }

  function filesFromEntry(entry) {
    if (entry.isFile) {
      return new Promise((resolve) => entry.file((file) => resolve([file]), () => resolve([])));
    }
    if (!entry.isDirectory) return Promise.resolve([]);
    const reader = entry.createReader();
    const files = [];
    return new Promise((resolve) => {
      const readBatch = () => {
        reader.readEntries(async (entries) => {
          if (!entries.length) {
            resolve(files);
            return;
          }
          for (const child of entries) files.push(...await filesFromEntry(child));
          readBatch();
        }, () => resolve(files));
      };
      readBatch();
    });
  }

  window.OpenJoeyScreens.SettingsScreen = SettingsScreen;
})();
