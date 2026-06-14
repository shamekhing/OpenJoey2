(function () {
  const Ui = window.OpenJoeyUi;
  const { CARD_ASPECT } = window.OpenJoeyDeckConstants;

  /**
   * Pure layout calculations for the deck editor.
   */
  function compute(app) {
    const margin = app.w < 520 || app.h < 520 ? 8 : 14;
    const gap = app.w < 520 || app.h < 520 ? 8 : 10;
    const top = app.chromeTop() + (margin === 8 ? 8 : 16);
    const bottom = app.chromeBottom() + (margin === 8 ? 8 : 14);
    const h = Math.max(160, app.h - top - bottom);
    const portrait = app.w <= 620 && app.h > app.w;
    const landscapePhone = app.h < 560 || app.w < 900;
    const mode = portrait ? "portrait" : landscapePhone ? "landscape" : "wide";
    let preview;
    let pool;
    let deck;

    if (portrait) {
      const w = app.w - margin * 2;
      const poolH = Math.max(210, Math.floor((h - gap) * 0.56));
      preview = { x: margin, y: top, w: 0, h: 0 };
      pool = { x: margin, y: top, w, h: poolH };
      deck = { x: margin, y: top + poolH + gap, w, h: h - poolH - gap };
    } else {
      const previewW = landscapePhone
        ? Math.max(126, Math.min(178, Math.floor(app.w * 0.18)))
        : Math.max(260, Math.floor(app.w * 0.25));
      const deckW = landscapePhone
        ? Math.max(218, Math.min(300, Math.floor(app.w * 0.32)))
        : Math.max(360, Math.floor(app.w * 0.32));
      const poolW = Math.max(190, app.w - margin * 2 - previewW - deckW);
      preview = { x: margin, y: top, w: previewW, h };
      pool = { x: margin + previewW, y: top, w: poolW, h };
      deck = { x: margin + previewW + poolW, y: top, w: app.w - margin - (margin + previewW + poolW), h };
    }

    return {
      mode,
      preview,
      pool,
      deck,
      search: {
        left: pool.x + 12,
        top: pool.y + 62,
        width: Math.max(120, pool.w - 24),
      },
    };
  }

  function hitList(rect, items, cursor, offset, grid, x, y) {
    // Map canvas coordinates back into the virtual list/grid index.
    if (x < rect.x || x > rect.x + rect.w || y < rect.y + offset || y > rect.y + rect.h) return -1;
    if (!grid) {
      const visible = visibleRows(rect, offset);
      const index = Ui.visibleStart(items.length, cursor, visible) + Math.floor((y - rect.y - offset) / 64);
      return index >= 0 && index < items.length ? index : -1;
    }

    const cols = gridCols(rect);
    const metrics = gridMetrics(rect, offset, cols);
    const start = Ui.visibleStart(items.length, cursor, metrics.visible);
    const col = Math.floor((x - rect.x - metrics.gap) / (metrics.cellW + metrics.gap));
    const row = Math.floor((y - rect.y - offset - metrics.gap) / (metrics.cellH + metrics.gap));
    const index = start + row * cols + col;
    return col >= 0 && col < cols && row >= 0 && index < items.length ? index : -1;
  }

  function visibleRows(rect, offset) {
    return Math.max(1, Math.floor((rect.h - offset - 8) / 64));
  }

  function gridCols(rect) {
    if (!rect) return 4;
    return Math.max(2, Math.min(4, Math.floor(rect.w / 82)));
  }

  function gridMetrics(rect, offset, cols = gridCols(rect)) {
    const gap = 8;
    const cellW = (rect.w - gap * (cols + 1)) / cols;
    const cardH = cellW * CARD_ASPECT;
    const cellH = cardH + 24;
    const visible = Math.max(cols, Math.floor((rect.h - offset) / (cellH + gap)) * cols);
    return { gap, cellW, cardH, cellH, visible };
  }

  window.OpenJoeyDeckEditorLayout = { compute, hitList, visibleRows, gridCols, gridMetrics };
})();
