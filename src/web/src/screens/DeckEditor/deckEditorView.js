(function () {
  const Ui = window.OpenJoeyUi;
  const Card = window.OpenJoeyCardDb;
  const Layout = window.OpenJoeyDeckEditorLayout;
  const Selectors = window.OpenJoeyDeckEditorSelectors;
  const { CARD_ASPECT, MAX_COPIES } = window.OpenJoeyDeckConstants;

  /**
   * Canvas rendering for the deck editor.
   */
  function draw(g, app, state, layout) {
    app.drawChrome("DECK EDITOR", Selectors.helpText(layout.mode));
    drawPreview(g, app, state, layout.preview);
    drawPool(g, app, state, layout.pool);
    drawDeck(g, app, state, layout.deck);
  }

  function panel(g, rect, title, badge, focused) {
    Ui.rect(g, rect, "rgba(12,16,20,.92)", focused ? "#f3d45b" : "#303946");
    g.fillStyle = "#f1f5f8";
    g.font = "700 17px system-ui";
    Ui.text(g, title, rect.x + 12, rect.y + 28, rect.w - 24);
    g.fillStyle = "#9faab7";
    g.font = "14px system-ui";
    Ui.text(g, badge, rect.x + 12, rect.y + 49, rect.w - 24);
  }

  function drawPreview(g, app, state, rect) {
    if (rect.w <= 0 || rect.h <= 0) return;
    panel(g, rect, "Preview", "", false);
    const card = Selectors.selectedCard(state, app.deck);
    const compact = rect.w < 210 || rect.h < 420;
    const w = compact ? Math.min(rect.w - 28, 94) : Math.min(rect.w - 48, 240);
    const h = w * CARD_ASPECT;
    const x = rect.x + (rect.w - w) / 2;
    const y = rect.y + 58;
    Ui.cardImage(g, app.images, card, x, y, w, h);
    if (!card) return;
    g.fillStyle = "#f1f5f8";
    g.font = compact ? "700 13px system-ui" : "700 18px system-ui";
    Ui.text(g, card.name, rect.x + 16, y + h + 32, rect.w - 32);
    g.fillStyle = Ui.kindColor(card.kind);
    g.font = compact ? "12px system-ui" : "14px system-ui";
    g.fillText(Card.kindTag(card.kind), rect.x + 16, y + h + 56);
    g.fillStyle = "#d8e0e8";
    if (compact) {
      g.font = "12px system-ui";
      Ui.text(g, card.kind === 0 ? Card.statsLine(card, true) : Card.kindName(card.kind), rect.x + 16, y + h + 76, rect.w - 32);
    } else {
      Ui.wrap(g, card.desc, rect.x + 16, y + h + 84, rect.w - 32, 19, 12);
    }
  }

  function drawPool(g, app, state, rect) {
    panel(g, rect, "Card Pool", Selectors.poolBadge(state), state.focusPool);
    drawRows(g, app, rect, state.filtered, state.poolCursor, state.focusPool, 108);
  }

  function drawDeck(g, app, state, rect) {
    const stats = app.deck.stats();
    panel(g, rect, state.deckGrid ? "Deck [Grid]" : "Deck [List]", Selectors.deckBadge(stats), !state.focusPool);
    drawStats(g, rect, stats);
    if (state.deckGrid) drawGrid(g, app, rect, app.deck.cards, state.deckCursor, !state.focusPool, 118);
    else drawRows(g, app, rect, app.deck.cards, state.deckCursor, !state.focusPool, 118);
  }

  function drawStats(g, rect, stats) {
    const labels = [[`MON ${stats.monsters}`, "#d06062"], [`SPL ${stats.spells}`, "#56c978"], [`TRP ${stats.traps}`, "#d86fac"]];
    const y = rect.y + 64;
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

  function drawRows(g, app, rect, items, cursor, focused, offset) {
    // Lists are virtualized around the cursor to keep drawing bounded.
    const visible = Layout.visibleRows(rect, offset);
    const start = Ui.visibleStart(items.length, cursor, visible);
    for (let i = 0; i < visible && start + i < items.length; i += 1) {
      drawRow(g, app, rect, items[start + i], start + i, cursor, focused, rect.y + offset + i * 64);
    }
  }

  function drawGrid(g, app, rect, items, cursor, focused, offset) {
    // Grid uses the same cursor-centered window as row lists.
    const cols = Layout.gridCols(rect);
    const metrics = Layout.gridMetrics(rect, offset, cols);
    const start = Ui.visibleStart(items.length, cursor, metrics.visible);
    for (let i = 0; i < metrics.visible && start + i < items.length; i += 1) {
      const index = start + i;
      const card = items[index];
      const x = rect.x + metrics.gap + (i % cols) * (metrics.cellW + metrics.gap);
      const y = rect.y + offset + metrics.gap + Math.floor(i / cols) * (metrics.cellH + metrics.gap);
      if (focused && index === cursor) {
        g.strokeStyle = "#f3d45b";
        g.strokeRect(x - 2, y - 2, metrics.cellW + 4, metrics.cardH + 4);
      }
      Ui.cardImage(g, app.images, card, x, y, metrics.cellW, metrics.cardH);
      g.fillStyle = "#9faab7";
      g.font = "12px system-ui";
      Ui.text(g, card.name, x, y + metrics.cardH + 16, metrics.cellW);
    }
  }

  function drawRow(g, app, rect, card, index, cursor, focused, y) {
    if (focused && index === cursor) {
      g.strokeStyle = "#f3d45b";
      g.strokeRect(rect.x + 6.5, y + 2.5, rect.w - 13, 59);
    }
    Ui.cardImage(g, app.images, card, rect.x + 12, y + 7, 35, 51);
    g.fillStyle = Ui.kindColor(card.kind);
    g.font = "700 12px system-ui";
    g.fillText(Card.kindTag(card.kind), rect.x + 58, y + 20);
    g.fillStyle = "#f1f5f8";
    g.font = "700 14px system-ui";
    Ui.text(g, card.name, rect.x + 58, y + 39, Math.max(40, rect.w - 120));
    g.fillStyle = "#9faab7";
    g.font = "12px system-ui";
    g.fillText(card.kind === 0 ? Card.statsLine(card, true) : Card.kindName(card.kind), rect.x + 58, y + 56);
    const copies = app.deck.countCopies(card.id);
    g.fillStyle = copies >= MAX_COPIES ? "#e96161" : copies > 0 ? "#52d07d" : "#8e99a6";
    g.textAlign = "right";
    g.fillText(`${copies}/${MAX_COPIES}`, rect.x + rect.w - 18, y + 24);
    g.textAlign = "left";
  }

  window.OpenJoeyDeckEditorView = { draw };
})();
