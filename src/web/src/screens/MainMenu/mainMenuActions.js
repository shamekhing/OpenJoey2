(function () {
  /**
   * Input handlers for the main menu.
   */
  function select(app, target) {
    if (target === "quit") app.status = "Quit is native-only in browser";
    else app.goto(target);
  }

  function key(app, state, items, event) {
    if (event.key === "ArrowDown") {
      event.preventDefault();
      state.cursor = (state.cursor + 1) % items.length;
    } else if (event.key === "ArrowUp") {
      event.preventDefault();
      state.cursor = (state.cursor + items.length - 1) % items.length;
    } else if (event.key === "Enter" || event.key === " ") {
      event.preventDefault();
      select(app, items[state.cursor][1]);
    }
  }

  function click(app, state, items, metrics, x, y) {
    const row = Math.floor((y - metrics.startY + metrics.rowH / 2) / metrics.rowH);
    if (row < 0 || row >= items.length) return;
    state.cursor = row;
    select(app, items[row][1]);
  }

  window.OpenJoeyMainMenuActions = { key, click };
})();
