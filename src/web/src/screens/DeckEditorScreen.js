(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Ui = window.OpenJoeyUi;
  const Card = window.OpenJoeyCardDb;

  const sortLabels = [
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
  const typeLabels = ["All", "Monster", "Spell", "Trap"];

  class DeckEditorScreen {
    constructor(app) {
      this.app = app;
      this.poolCursor = 0;
      this.deckCursor = 0;
      this.focusPool = true;
      this.deckGrid = false;
      this.sortMode = 0;
      this.typeFilter = 0;
      this.query = "";
      this.filtered = [];
      this.app.cardDb.rebuildSort(this.sortMode);
      this.rebuild();
      this.app.searchInput.value = "";
      this.app.searchInput.style.display = "block";
      this.inputHandler = () => {
        this.query = this.app.searchInput.value;
        this.poolCursor = 0;
        this.rebuild();
      };
      this.app.searchInput.addEventListener("input", this.inputHandler);
    }

    layout() {
      const margin = this.app.w < 520 || this.app.h < 520 ? 8 : 14;
      const gap = this.app.w < 520 || this.app.h < 520 ? 8 : 10;
      const top = this.app.chromeTop() + (margin === 8 ? 8 : 16);
      const bottom = this.app.chromeBottom() + (margin === 8 ? 8 : 14);
      const h = Math.max(160, this.app.h - top - bottom);
      const portrait = this.app.w <= 620 && this.app.h > this.app.w;
      const landscapePhone = this.app.h < 560 || this.app.w < 900;
      this.layoutMode = portrait ? "portrait" : landscapePhone ? "landscape" : "wide";

      if (portrait) {
        const w = this.app.w - margin * 2;
        const poolH = Math.max(210, Math.floor((h - gap) * 0.56));
        this.preview = { x: margin, y: top, w: 0, h: 0 };
        this.pool = { x: margin, y: top, w, h: poolH };
        this.deck = { x: margin, y: top + poolH + gap, w, h: h - poolH - gap };
      } else {
        const previewW = landscapePhone
          ? Math.max(126, Math.min(178, Math.floor(this.app.w * 0.18)))
          : Math.max(260, Math.floor(this.app.w * 0.25));
        const deckW = landscapePhone
          ? Math.max(218, Math.min(300, Math.floor(this.app.w * 0.32)))
          : Math.max(360, Math.floor(this.app.w * 0.32));
        const poolW = Math.max(190, this.app.w - margin * 2 - previewW - deckW);
        this.preview = { x: margin, y: top, w: previewW, h };
        this.pool = { x: margin + previewW, y: top, w: poolW, h };
        this.deck = { x: margin + previewW + poolW, y: top, w: this.app.w - margin - (margin + previewW + poolW), h };
      }
      this.app.searchInput.style.left = `${this.pool.x + 12}px`;
      this.app.searchInput.style.top = `${this.pool.y + 62}px`;
      this.app.searchInput.style.width = `${Math.max(120, this.pool.w - 24)}px`;
    }

    dispose() {
      this.app.searchInput.removeEventListener("input", this.inputHandler);
    }

    rebuild() {
      this.filtered = this.app.cardDb.filter(this.query, this.typeFilter);
      this.clamp();
    }

    clamp() {
      this.poolCursor = Math.max(0, Math.min(this.poolCursor, this.filtered.length - 1));
      this.deckCursor = Math.max(0, Math.min(this.deckCursor, this.app.deck.cards.length - 1));
    }

    selected() {
      if (this.focusPool) return this.filtered[this.poolCursor] || null;
      return this.app.deck.cards[this.deckCursor] || null;
    }

    key(event) {
      if (event.target === this.app.searchInput && event.key !== "Enter" && event.key !== "Escape") return;
      const block = ["Tab", "ArrowDown", "ArrowUp", "ArrowLeft", "ArrowRight", "PageDown", "PageUp", "Enter", "Delete", "Backspace"];
      if (block.includes(event.key)) event.preventDefault();
      const step = !this.focusPool && this.deckGrid ? this.gridCols(this.deck) : 1;
      if (event.key === "Escape") this.app.goto("menu");
      else if (event.key === "Tab") this.focusPool = !this.focusPool;
      else if (event.key === "ArrowDown") this.move(step);
      else if (event.key === "ArrowUp") this.move(-step);
      else if (event.key === "PageDown") this.move(step * 8);
      else if (event.key === "PageUp") this.move(-step * 8);
      else if (event.key === "ArrowRight") this.focusPool ? (this.focusPool = false) : this.move(1);
      else if (event.key === "ArrowLeft") !this.focusPool && this.deckGrid ? this.move(-1) : (this.focusPool = true);
      else if (event.key === "Enter") this.add();
      else if (event.key === "Delete" || event.key === "Backspace" || event.key.toLowerCase() === "d") this.remove();
      else if (event.key.toLowerCase() === "o") this.sort();
      else if (event.key.toLowerCase() === "t") this.type();
      else if (event.key.toLowerCase() === "g") this.deckGrid = !this.deckGrid;
      else if (event.key.toLowerCase() === "c") this.clear();
      else if (event.key.toLowerCase() === "s") this.save();
      else if (event.key.toLowerCase() === "l") this.load();
      else if (event.key.toLowerCase() === "f") this.duel();
      this.clamp();
    }

    move(delta) {
      if (this.focusPool) this.poolCursor += delta;
      else this.deckCursor += delta;
      this.clamp();
    }

    add() {
      const card = this.filtered[this.poolCursor];
      if (!card) return;
      if (this.app.deck.add(card)) {
        this.deckCursor = this.app.deck.cards.length - 1;
        this.app.status = `Added: ${card.name}`;
        this.persist();
      } else {
        this.app.status = this.app.deck.countCopies(card.id) >= 3 ? `Max 3 copies of ${card.name}` : "Deck full (60 cards max)";
      }
    }

    remove() {
      if (!this.app.deck.cards.length) return;
      const card = this.app.deck.cards[this.deckCursor];
      if (this.app.deck.removeAt(this.deckCursor)) {
        this.app.status = `Removed: ${card.name}`;
        this.persist();
      }
      this.clamp();
    }

    sort() {
      this.sortMode = (this.sortMode + 1) % sortLabels.length;
      this.app.cardDb.rebuildSort(this.sortMode);
      this.poolCursor = 0;
      this.rebuild();
      this.app.status = `Sort: ${sortLabels[this.sortMode]}`;
    }

    type() {
      this.typeFilter = (this.typeFilter + 1) % typeLabels.length;
      this.poolCursor = 0;
      this.rebuild();
      this.app.status = `Filter: ${typeLabels[this.typeFilter]}`;
    }

    clear() {
      this.app.deck.clear();
      this.deckCursor = 0;
      this.persist();
      this.app.status = "Deck cleared";
    }

    persist() {
      localStorage.setItem("openjoey2.src2.deck", JSON.stringify(this.app.deck.cards.map((card) => card.id)));
    }

    load() {
      const ids = JSON.parse(localStorage.getItem("openjoey2.src2.deck") || "[]");
      this.app.deck.clear();
      for (const id of ids) {
        const card = this.app.cardDb.byId.get(id);
        if (card) this.app.deck.add(card);
      }
      this.deckCursor = 0;
      this.app.status = this.app.deck.cards.length ? "Loaded deck" : "No saved deck";
    }

    save() {
      this.persist();
      this.app.status = "Saved deck";
    }

    duel() {
      if (!this.app.deck.canDuel()) {
        this.app.status = `Need ${40 - this.app.deck.cards.length} more cards`;
        return;
      }
      this.app.goto("duel");
    }

    click(x, y) {
      const pool = this.hitRows(this.pool, this.filtered, this.poolCursor, 108, false, x, y);
      if (pool >= 0) {
        this.focusPool = true;
        this.poolCursor = pool;
        return;
      }
      const deck = this.hitRows(this.deck, this.app.deck.cards, this.deckCursor, 118, this.deckGrid, x, y);
      if (deck >= 0) {
        this.focusPool = false;
        this.deckCursor = deck;
      }
    }

    doubleClick() {
      if (this.focusPool) this.add();
      else this.remove();
    }

    hitRows(rect, items, cursor, offset, grid, x, y) {
      if (x < rect.x || x > rect.x + rect.w || y < rect.y + offset || y > rect.y + rect.h) return -1;
      if (grid) {
        const cols = this.gridCols(rect);
        const gap = 8;
        const cellW = (rect.w - gap * (cols + 1)) / cols;
        const cardH = cellW * 86 / 59;
        const cellH = cardH + 24;
        const visible = Math.max(cols, Math.floor((rect.h - offset) / (cellH + gap)) * cols);
        const start = Ui.visibleStart(items.length, cursor, visible);
        const col = Math.floor((x - rect.x - gap) / (cellW + gap));
        const row = Math.floor((y - rect.y - offset - gap) / (cellH + gap));
        if (col < 0 || col >= cols || row < 0) return -1;
        return start + row * cols + col;
      }
      const visible = Math.max(1, Math.floor((rect.h - offset - 8) / 64));
      return Ui.visibleStart(items.length, cursor, visible) + Math.floor((y - rect.y - offset) / 64);
    }

    draw(g) {
      this.app.drawChrome("DECK EDITOR", this.layoutMode === "wide"
        ? "[TAB] switch [Arrows] navigate [ENTER] add [DEL/D] remove [O] sort [T] filter [G] grid/list [C] clear [S] save [L] load [F] duel [ESC] menu"
        : "[TAB] switch [ENTER] add [DEL/D] remove [O/T] sort/filter [G] grid [F] duel [ESC] menu");
      this.drawPreview(g);
      this.drawPool(g);
      this.drawDeck(g);
    }

    panel(g, rect, title, badge, focused) {
      Ui.rect(g, rect, "rgba(12,16,20,.92)", focused ? "#f3d45b" : "#303946");
      g.fillStyle = "#f1f5f8";
      g.font = "700 17px system-ui";
      Ui.text(g, title, rect.x + 12, rect.y + 28, rect.w - 24);
      g.fillStyle = "#9faab7";
      g.font = "14px system-ui";
      Ui.text(g, badge, rect.x + 12, rect.y + 49, rect.w - 24);
    }

    drawPreview(g) {
      if (this.preview.w <= 0 || this.preview.h <= 0) return;
      this.panel(g, this.preview, "Preview", "", false);
      const card = this.selected();
      const compact = this.preview.w < 210 || this.preview.h < 420;
      const w = compact ? Math.min(this.preview.w - 28, 94) : Math.min(this.preview.w - 48, 240);
      const h = w * 86 / 59;
      const x = this.preview.x + (this.preview.w - w) / 2;
      const y = this.preview.y + 58;
      Ui.cardImage(g, this.app.images, card, x, y, w, h);
      if (!card) return;
      g.fillStyle = "#f1f5f8";
      g.font = compact ? "700 13px system-ui" : "700 18px system-ui";
      Ui.text(g, card.name, this.preview.x + 16, y + h + 32, this.preview.w - 32);
      g.fillStyle = Ui.kindColor(card.kind);
      g.font = compact ? "12px system-ui" : "14px system-ui";
      g.fillText(Card.kindTag(card.kind), this.preview.x + 16, y + h + 56);
      g.fillStyle = "#d8e0e8";
      if (compact) {
        g.font = "12px system-ui";
        Ui.text(g, card.kind === 0 ? `L${card.level} ${card.atk}/${card.def}` : Card.kindName(card.kind), this.preview.x + 16, y + h + 76, this.preview.w - 32);
      } else {
        Ui.wrap(g, card.desc, this.preview.x + 16, y + h + 84, this.preview.w - 32, 19, 12);
      }
    }

    drawPool(g) {
      this.panel(g, this.pool, "Card Pool", `${sortLabels[this.sortMode]} [${typeLabels[this.typeFilter]}] ${this.filtered.length} cards`, this.focusPool);
      this.drawRows(g, this.pool, this.filtered, this.poolCursor, this.focusPool, 108);
    }

    drawDeck(g) {
      const stats = this.app.deck.stats();
      const badge = stats.total < 40 ? `${stats.total}/40 cards` : `${stats.total}/60 cards`;
      this.panel(g, this.deck, this.deckGrid ? "Deck [Grid]" : "Deck [List]", badge, !this.focusPool);
      this.drawStats(g, stats);
      if (this.deckGrid) this.drawGrid(g, this.deck, this.app.deck.cards, this.deckCursor, !this.focusPool, 118);
      else this.drawRows(g, this.deck, this.app.deck.cards, this.deckCursor, !this.focusPool, 118);
    }

    drawStats(g, stats) {
      const labels = [[`MON ${stats.monsters}`, "#d06062"], [`SPL ${stats.spells}`, "#56c978"], [`TRP ${stats.traps}`, "#d86fac"]];
      const y = this.deck.y + 64;
      const w = (this.deck.w - 48) / 3;
      for (let i = 0; i < 3; i += 1) {
        const x = this.deck.x + 12 + i * (w + 12);
        g.strokeStyle = "#303946";
        g.strokeRect(x, y, w, 30);
        g.fillStyle = labels[i][1];
        g.font = "12px system-ui";
        g.textAlign = "center";
        g.fillText(labels[i][0], x + w / 2, y + 20);
      }
      g.textAlign = "left";
    }

    drawRows(g, rect, items, cursor, focused, offset) {
      const visible = Math.max(1, Math.floor((rect.h - offset - 8) / 64));
      const start = Ui.visibleStart(items.length, cursor, visible);
      for (let i = 0; i < visible && start + i < items.length; i += 1) {
        const index = start + i;
        const card = items[index];
        const y = rect.y + offset + i * 64;
        if (focused && index === cursor) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(rect.x + 6.5, y + 2.5, rect.w - 13, 59);
        }
        Ui.cardImage(g, this.app.images, card, rect.x + 12, y + 7, 35, 51);
        g.fillStyle = Ui.kindColor(card.kind);
        g.font = "700 12px system-ui";
        g.fillText(Card.kindTag(card.kind), rect.x + 58, y + 20);
        g.fillStyle = "#f1f5f8";
        g.font = "700 14px system-ui";
        Ui.text(g, card.name, rect.x + 58, y + 39, Math.max(40, rect.w - 120));
        g.fillStyle = "#9faab7";
        g.font = "12px system-ui";
        g.fillText(card.kind === 0 ? `L${card.level} ${card.atk}/${card.def}` : Card.kindName(card.kind), rect.x + 58, y + 56);
        const copies = this.app.deck.countCopies(card.id);
        g.fillStyle = copies >= 3 ? "#e96161" : copies > 0 ? "#52d07d" : "#8e99a6";
        g.textAlign = "right";
        g.fillText(`${copies}/3`, rect.x + rect.w - 18, y + 24);
        g.textAlign = "left";
      }
    }

    drawGrid(g, rect, items, cursor, focused, offset) {
      const cols = this.gridCols(rect);
      const gap = 8;
      const cellW = (rect.w - gap * (cols + 1)) / cols;
      const cardH = cellW * 86 / 59;
      const cellH = cardH + 24;
      const visible = Math.max(cols, Math.floor((rect.h - offset) / (cellH + gap)) * cols);
      const start = Ui.visibleStart(items.length, cursor, visible);
      for (let i = 0; i < visible && start + i < items.length; i += 1) {
        const index = start + i;
        const card = items[index];
        const x = rect.x + gap + (i % cols) * (cellW + gap);
        const y = rect.y + offset + gap + Math.floor(i / cols) * (cellH + gap);
        if (focused && index === cursor) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(x - 2, y - 2, cellW + 4, cardH + 4);
        }
        Ui.cardImage(g, this.app.images, card, x, y, cellW, cardH);
        g.fillStyle = "#9faab7";
        g.font = "12px system-ui";
        Ui.text(g, card.name, x, y + cardH + 16, cellW);
      }
    }

    gridCols(rect) {
      if (!rect) return 4;
      return Math.max(2, Math.min(4, Math.floor(rect.w / 82)));
    }
  }

  window.OpenJoeyScreens.DeckEditorScreen = DeckEditorScreen;
})();
