(function () {
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

  const { imageUrl, kindName, kindTag } = window.OpenJoeyCardDb;

  class CanvasDeckEditor {
    constructor(canvas, searchInput, cardDb, deckCore, backend) {
      this.canvas = canvas;
      this.ctx = canvas.getContext("2d", { alpha: false });
      this.searchInput = searchInput;
      this.cardDb = cardDb;
      this.deck = deckCore;
      this.backend = backend;
      this.images = new window.OpenJoeyImageCache.ImageCache(120);
      this.poolCursor = 0;
      this.deckCursor = 0;
      this.focusPool = true;
      this.deckGrid = false;
      this.sortMode = 0;
      this.typeFilter = 0;
      this.query = "";
      this.status = `Ready (${backend})`;
      this.filtered = [];
      this.mouse = { x: 0, y: 0 };
      this.cardDb.rebuildSort(this.sortMode);
      this.rebuildFilter();
      this.bind();
      this.resize();
      requestAnimationFrame(() => this.frame());
    }

    bind() {
      window.addEventListener("resize", () => this.resize());
      document.addEventListener("keydown", (event) => this.key(event));
      this.searchInput.addEventListener("input", () => {
        this.query = this.searchInput.value;
        this.poolCursor = 0;
        this.rebuildFilter();
      });
      this.canvas.addEventListener("mousemove", (event) => {
        const rect = this.canvas.getBoundingClientRect();
        this.mouse.x = (event.clientX - rect.left) * (this.canvas.width / rect.width);
        this.mouse.y = (event.clientY - rect.top) * (this.canvas.height / rect.height);
      });
      this.canvas.addEventListener("click", () => this.click());
      this.canvas.addEventListener("dblclick", () => {
        if (this.focusPool) this.addSelected();
        else this.removeSelected();
      });
    }

    resize() {
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      this.canvas.width = Math.floor(window.innerWidth * dpr);
      this.canvas.height = Math.floor(window.innerHeight * dpr);
      this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      this.w = window.innerWidth;
      this.h = window.innerHeight;
      this.layout();
    }

    layout() {
      const top = 56;
      const bottom = 40;
      const x = 14;
      const y = top + 16;
      const h = this.h - top - bottom - 24;
      const previewW = Math.max(260, Math.floor(this.w * 0.25));
      const deckW = Math.max(360, Math.floor(this.w * 0.32));
      const poolW = this.w - previewW - deckW - 28;
      this.previewRect = { x, y, w: previewW, h };
      this.poolRect = { x: x + previewW, y, w: poolW, h };
      this.deckRect = { x: x + previewW + poolW, y, w: deckW, h };
      this.searchInput.style.left = `${this.poolRect.x + 12}px`;
      this.searchInput.style.top = `${this.poolRect.y + 62}px`;
      this.searchInput.style.width = `${this.poolRect.w - 24}px`;
    }

    rebuildFilter() {
      this.filtered = this.cardDb.filter(this.query, this.typeFilter);
      this.clamp();
    }

    clamp() {
      this.poolCursor = Math.max(0, Math.min(this.poolCursor, this.filtered.length - 1));
      this.deckCursor = Math.max(0, Math.min(this.deckCursor, this.deck.cards.length - 1));
    }

    selectedCard() {
      if (this.focusPool) return this.filtered[this.poolCursor] || null;
      return this.deck.cards[this.deckCursor] || null;
    }

    key(event) {
      if (event.target === this.searchInput && event.key !== "Enter" && event.key !== "Escape") {
        return;
      }
      const block = ["Tab", "ArrowDown", "ArrowUp", "ArrowLeft", "ArrowRight", "PageDown", "PageUp", "Enter", "Delete", "Backspace"];
      if (block.includes(event.key)) event.preventDefault();
      const step = !this.focusPool && this.deckGrid ? 4 : 1;
      if (event.key === "Tab") this.focusPool = !this.focusPool;
      else if (event.key === "ArrowDown") this.move(step);
      else if (event.key === "ArrowUp") this.move(-step);
      else if (event.key === "PageDown") this.move(step * 8);
      else if (event.key === "PageUp") this.move(-step * 8);
      else if (event.key === "ArrowRight") this.focusPool ? (this.focusPool = false) : this.move(1);
      else if (event.key === "ArrowLeft") !this.focusPool && this.deckGrid ? this.move(-1) : (this.focusPool = true);
      else if (event.key === "Enter") this.addSelected();
      else if (event.key === "Delete" || event.key === "Backspace" || event.key.toLowerCase() === "d") this.removeSelected();
      else if (event.key.toLowerCase() === "o") this.cycleSort();
      else if (event.key.toLowerCase() === "t") this.cycleType();
      else if (event.key.toLowerCase() === "g") this.toggleGrid();
      else if (event.key.toLowerCase() === "c") this.clearDeck();
      else if (event.key.toLowerCase() === "s") this.saveDeck();
      else if (event.key.toLowerCase() === "l") this.loadDeck();
      else if (event.key.toLowerCase() === "f") this.duelGate();
      else if (event.key === "Escape") this.status = "";
      this.clamp();
    }

    move(delta) {
      if (this.focusPool) this.poolCursor += delta;
      else this.deckCursor += delta;
      this.clamp();
    }

    cycleSort() {
      this.sortMode = (this.sortMode + 1) % sortLabels.length;
      this.cardDb.rebuildSort(this.sortMode);
      this.poolCursor = 0;
      this.rebuildFilter();
      this.status = `Sort: ${sortLabels[this.sortMode]}`;
    }

    cycleType() {
      this.typeFilter = (this.typeFilter + 1) % typeLabels.length;
      this.poolCursor = 0;
      this.rebuildFilter();
      this.status = `Filter: ${typeLabels[this.typeFilter]}`;
    }

    toggleGrid() {
      this.deckGrid = !this.deckGrid;
      this.status = this.deckGrid ? "Deck: grid view" : "Deck: list view";
    }

    addSelected() {
      const card = this.filtered[this.poolCursor];
      if (!card) return;
      if (this.deck.add(card)) {
        this.deckCursor = this.deck.cards.length - 1;
        this.status = `Added: ${card.name}`;
        this.persist();
      } else if (this.deck.countCopies(card.id) >= 3) {
        this.status = `Max 3 copies of ${card.name}`;
      } else {
        this.status = "Deck full (60 cards max)";
      }
    }

    removeSelected() {
      if (!this.deck.cards.length) return;
      const card = this.deck.cards[this.deckCursor];
      if (this.deck.removeAt(this.deckCursor)) {
        this.status = `Removed: ${card.name}`;
        this.persist();
      }
      this.clamp();
    }

    clearDeck() {
      this.deck.clear();
      this.deckCursor = 0;
      this.status = "Deck cleared";
      this.persist();
    }

    persist() {
      localStorage.setItem("openjoey2.src2.deck", JSON.stringify(this.deck.cards.map((card) => card.id)));
    }

    loadDeck() {
      const ids = JSON.parse(localStorage.getItem("openjoey2.src2.deck") || "[]");
      this.deck.clear();
      for (const id of ids) {
        const card = this.cardDb.byId.get(id);
        if (card) this.deck.add(card);
      }
      this.deckCursor = 0;
      this.status = this.deck.cards.length ? "Loaded deck" : "No saved deck";
    }

    saveDeck() {
      this.persist();
      const text = this.deck.cards.map((card) => card.id).join("\n");
      const url = URL.createObjectURL(new Blob([text], { type: "text/plain" }));
      const link = document.createElement("a");
      link.href = url;
      link.download = "default.txt";
      link.click();
      URL.revokeObjectURL(url);
      this.status = "Saved deck";
    }

    duelGate() {
      const need = 40 - this.deck.cards.length;
      this.status = this.deck.canDuel() ? "Deck ready for duel" : `Need ${need} more cards`;
    }

    click() {
      const hitPool = this.hitList(this.poolRect, this.filtered, false);
      if (hitPool >= 0) {
        this.focusPool = true;
        this.poolCursor = hitPool;
        return;
      }
      const hitDeck = this.hitList(this.deckRect, this.deck.cards, this.deckGrid);
      if (hitDeck >= 0) {
        this.focusPool = false;
        this.deckCursor = hitDeck;
      }
    }

    hitList(rect, items, grid) {
      const listY = rect.y + (rect === this.poolRect ? 108 : 118);
      if (this.mouse.x < rect.x || this.mouse.x > rect.x + rect.w || this.mouse.y < listY || this.mouse.y > rect.y + rect.h) return -1;
      if (grid) {
        const cols = 4;
        const gap = 8;
        const cellW = (rect.w - gap * (cols + 1)) / cols;
        const cellH = cellW * 1.45 + 22;
        const col = Math.floor((this.mouse.x - rect.x - gap) / (cellW + gap));
        const row = Math.floor((this.mouse.y - listY - gap) / (cellH + gap));
        const index = this.visibleStart(items.length, this.deckCursor, 24) + row * cols + col;
        return col >= 0 && col < cols && index < items.length ? index : -1;
      }
      const rowH = 64;
      const index = this.visibleStart(items.length, rect === this.poolRect ? this.poolCursor : this.deckCursor) + Math.floor((this.mouse.y - listY) / rowH);
      return index >= 0 && index < items.length ? index : -1;
    }

    frame() {
      this.draw();
      requestAnimationFrame(() => this.frame());
    }

    draw() {
      const g = this.ctx;
      g.clearRect(0, 0, this.w, this.h);
      g.fillStyle = "#0a0d11";
      g.fillRect(0, 0, this.w, this.h);
      this.drawTop();
      this.drawPreview();
      this.drawPool();
      this.drawDeck();
      this.drawFooter();
    }

    drawTop() {
      const g = this.ctx;
      g.fillStyle = "#11161c";
      g.fillRect(0, 0, this.w, 56);
      g.strokeStyle = "#303946";
      g.beginPath();
      g.moveTo(0, 55.5);
      g.lineTo(this.w, 55.5);
      g.stroke();
      g.fillStyle = "#f1f5f8";
      g.font = "700 21px system-ui";
      g.fillText("DECK EDITOR", 16, 36);
      g.fillStyle = "#9faab7";
      g.font = "14px system-ui";
      g.textAlign = "right";
      g.fillText(this.status, this.w - 16, 35);
      g.textAlign = "left";
    }

    drawPanel(rect, title, badge, focused) {
      const g = this.ctx;
      g.fillStyle = "rgba(12, 16, 20, 0.92)";
      g.fillRect(rect.x, rect.y, rect.w, rect.h);
      g.strokeStyle = focused ? "#f3d45b" : "#303946";
      g.strokeRect(rect.x + 0.5, rect.y + 0.5, rect.w - 1, rect.h - 1);
      g.fillStyle = "#f1f5f8";
      g.font = "700 17px system-ui";
      g.fillText(title, rect.x + 12, rect.y + 28);
      g.fillStyle = "#9faab7";
      g.font = "14px system-ui";
      g.fillText(badge, rect.x + 12, rect.y + 49);
    }

    drawPreview() {
      const card = this.selectedCard();
      const r = this.previewRect;
      this.drawPanel(r, "Preview", "", false);
      const g = this.ctx;
      const cardW = Math.min(r.w - 48, 260);
      const cardH = cardW * 86 / 59;
      const x = r.x + (r.w - cardW) / 2;
      const y = r.y + 58;
      this.drawCardImage(card, x, y, cardW, cardH);
      if (!card) {
        g.fillStyle = "#f1f5f8";
        g.font = "700 18px system-ui";
        g.fillText("Select a card", r.x + 16, y + cardH + 32);
        return;
      }
      g.fillStyle = "#f1f5f8";
      g.font = "700 18px system-ui";
      this.text(card.name, r.x + 16, y + cardH + 32, r.w - 32);
      g.fillStyle = this.kindColor(card.kind);
      g.font = "14px system-ui";
      g.fillText(kindTag(card.kind), r.x + 16, y + cardH + 58);
      g.fillStyle = "#9faab7";
      if (card.kind === 0) g.fillText(`Level ${card.level}  ATK ${card.atk}  DEF ${card.def}`, r.x + 16, y + cardH + 78);
      g.fillStyle = "#d8e0e8";
      g.font = "14px system-ui";
      this.wrap(card.desc, r.x + 16, y + cardH + 106, r.w - 32, 19, 13);
    }

    drawPool() {
      const badge = `${sortLabels[this.sortMode]} [${typeLabels[this.typeFilter]}] ${this.filtered.length} cards`;
      this.drawPanel(this.poolRect, "Card Pool", badge, this.focusPool);
      this.drawRows(this.poolRect, this.filtered, this.poolCursor, this.focusPool, 108);
    }

    drawDeck() {
      const stats = this.deck.stats();
      const need = Math.max(0, 40 - stats.total);
      const badge = need ? `${stats.total}/40 cards` : `${stats.total}/60 cards`;
      this.drawPanel(this.deckRect, this.deckGrid ? "Deck [Grid]" : "Deck [List]", badge, !this.focusPool);
      this.drawStats(this.deckRect, stats);
      if (this.deckGrid) this.drawGrid(this.deckRect, this.deck.cards, this.deckCursor, !this.focusPool);
      else this.drawRows(this.deckRect, this.deck.cards, this.deckCursor, !this.focusPool, 118);
    }

    drawStats(rect, stats) {
      const g = this.ctx;
      const y = rect.y + 64;
      const labels = [
        [`MON ${stats.monsters}`, "#d06062"],
        [`SPL ${stats.spells}`, "#56c978"],
        [`TRP ${stats.traps}`, "#d86fac"],
      ];
      const w = (rect.w - 48) / 3;
      for (let i = 0; i < 3; i += 1) {
        const x = rect.x + 12 + i * (w + 12);
        g.strokeStyle = "#303946";
        g.strokeRect(x, y, w, 30);
        g.fillStyle = labels[i][1];
        g.font = "12px system-ui";
        g.textAlign = "center";
        g.fillText(labels[i][0], x + w / 2, y + 20);
      }
      g.textAlign = "left";
    }

    drawRows(rect, items, cursor, focused, offsetY) {
      const g = this.ctx;
      const rowH = 64;
      const listY = rect.y + offsetY;
      const visible = Math.floor((rect.h - offsetY - 8) / rowH);
      const start = this.visibleStart(items.length, cursor, visible);
      for (let i = 0; i < visible && start + i < items.length; i += 1) {
        const index = start + i;
        const card = items[index];
        const y = listY + i * rowH;
        if (focused && index === cursor) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(rect.x + 6.5, y + 2.5, rect.w - 13, rowH - 5);
        }
        this.drawCardImage(card, rect.x + 12, y + 7, 35, 51);
        g.fillStyle = this.kindColor(card.kind);
        g.font = "700 12px system-ui";
        g.fillText(kindTag(card.kind), rect.x + 58, y + 20);
        g.fillStyle = "#f1f5f8";
        g.font = "700 14px system-ui";
        this.text(card.name, rect.x + 58, y + 39, rect.w - 120);
        g.fillStyle = "#9faab7";
        g.font = "12px system-ui";
        g.fillText(card.kind === 0 ? `L${card.level} ${card.atk}/${card.def}` : kindName(card.kind), rect.x + 58, y + 56);
        const copies = this.deck.countCopies(card.id);
        g.fillStyle = copies >= 3 ? "#e96161" : copies > 0 ? "#52d07d" : "#8e99a6";
        g.textAlign = "right";
        g.fillText(`${copies}/3`, rect.x + rect.w - 18, y + 24);
        g.textAlign = "left";
      }
    }

    drawGrid(rect, items, cursor, focused) {
      const g = this.ctx;
      const cols = 4;
      const gap = 8;
      const y0 = rect.y + 118;
      const cellW = (rect.w - gap * (cols + 1)) / cols;
      const cardH = cellW * 86 / 59;
      const cellH = cardH + 24;
      const rows = Math.max(1, Math.floor((rect.h - 126) / (cellH + gap)));
      const visible = rows * cols;
      const start = this.visibleStart(items.length, cursor, visible);
      for (let i = 0; i < visible && start + i < items.length; i += 1) {
        const index = start + i;
        const card = items[index];
        const col = i % cols;
        const row = Math.floor(i / cols);
        const x = rect.x + gap + col * (cellW + gap);
        const y = y0 + gap + row * (cellH + gap);
        if (focused && index === cursor) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(x - 2, y - 2, cellW + 4, cardH + 4);
        }
        this.drawCardImage(card, x, y, cellW, cardH);
        g.fillStyle = "#9faab7";
        g.font = "12px system-ui";
        this.text(card.name, x, y + cardH + 16, cellW);
      }
    }

    drawCardImage(card, x, y, w, h) {
      const g = this.ctx;
      if (!card) {
        g.fillStyle = "#5a2f22";
        g.fillRect(x, y, w, h);
        g.strokeStyle = "#c59d71";
        g.strokeRect(x, y, w, h);
        return;
      }
      const img = this.images.get(imageUrl(card));
      if (img) g.drawImage(img, x, y, w, h);
      else {
        g.fillStyle = card.kind === 1 ? "#163d2f" : card.kind === 2 ? "#48233a" : "#4b2f20";
        g.fillRect(x, y, w, h);
      }
      g.strokeStyle = "#303946";
      g.strokeRect(x, y, w, h);
    }

    drawFooter() {
      const g = this.ctx;
      g.fillStyle = "#11161c";
      g.fillRect(0, this.h - 40, this.w, 40);
      g.strokeStyle = "#303946";
      g.beginPath();
      g.moveTo(0, this.h - 39.5);
      g.lineTo(this.w, this.h - 39.5);
      g.stroke();
      g.fillStyle = "#9faab7";
      g.font = "12px system-ui";
      g.fillText("[TAB] switch [Arrows] navigate [PgUp/Dn] fast scroll [ENTER] add [DEL/D] remove [O] sort [T] filter [G] grid/list [C] clear [S] save [L] load [F] duel (40+)", 12, this.h - 16);
    }

    visibleStart(total, cursor, max = 18) {
      if (total <= max) return 0;
      return Math.max(0, Math.min(cursor - Math.floor(max / 2), total - max));
    }

    kindColor(kind) {
      if (kind === 1) return "#56c978";
      if (kind === 2) return "#d86fac";
      return "#d06062";
    }

    text(value, x, y, maxWidth) {
      const g = this.ctx;
      let s = value;
      while (s.length && g.measureText(s).width > maxWidth) s = s.slice(0, -1);
      if (s.length < value.length) s = `${s.slice(0, -1)}~`;
      g.fillText(s, x, y);
    }

    wrap(value, x, y, maxWidth, lineH, maxLines) {
      const words = value.split(/\s+/);
      let line = "";
      let lines = 0;
      for (const word of words) {
        const test = line ? `${line} ${word}` : word;
        if (this.ctx.measureText(test).width > maxWidth && line) {
          this.ctx.fillText(line, x, y + lines * lineH);
          line = word;
          lines += 1;
          if (lines >= maxLines) return;
        } else {
          line = test;
        }
      }
      if (line && lines < maxLines) this.ctx.fillText(line, x, y + lines * lineH);
    }
  }

  window.OpenJoeyCanvasDeckEditor = { CanvasDeckEditor };
})();
