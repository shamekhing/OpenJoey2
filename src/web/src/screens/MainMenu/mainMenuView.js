(function () {
  /**
   * View-only helpers for the main menu.
   */
  function metrics(app, itemCount) {
    const short = app.h < 460;
    const titleSize = Math.max(34, Math.min(short ? 44 : 64, Math.floor(app.w * 0.15)));
    const itemSize = Math.max(20, Math.min(short ? 24 : 28, Math.floor(app.w * 0.072)));
    const rowH = Math.max(34, itemSize + 14);
    const top = app.chromeTop();
    const bottom = app.chromeBottom();
    const titleY = Math.max(top + titleSize + 10, Math.floor(app.h * (short ? 0.27 : 0.25)));
    const menuH = rowH * itemCount;
    const startY = Math.min(
      app.h - bottom - menuH + rowH * 0.78,
      Math.max(titleY + rowH * 1.35, Math.floor(app.h * (short ? 0.47 : 0.5))),
    );
    return { titleSize, itemSize, rowH, titleY, startY };
  }

  function draw(g, app, state, items) {
    app.drawChrome("OpenJoey 2", "UP/DOWN to navigate, ENTER to select");
    const m = metrics(app, items.length);
    g.fillStyle = "#d7b84d";
    g.font = `700 ${m.titleSize}px system-ui`;
    g.textAlign = "center";
    g.fillText("OpenJoey2", app.w / 2, m.titleY, app.w - 28);
    g.font = `${m.itemSize}px system-ui`;
    for (let i = 0; i < items.length; i += 1) {
      const y = m.startY + i * m.rowH;
      g.fillStyle = i === state.cursor ? "#f3d45b" : "#d7dde6";
      g.fillText(items[i][0], app.w / 2, y);
      if (i === state.cursor) {
        const markerX = Math.max(24, app.w / 2 - Math.min(150, app.w * 0.36));
        g.fillText(">", markerX, y);
      }
    }
    g.textAlign = "left";
  }

  window.OpenJoeyMainMenuView = { draw, metrics };
})();
