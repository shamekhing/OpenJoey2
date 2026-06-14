(function () {
  /**
   * Cursor state for the duel screen.
   *
   * `mode` selects the active area, `player` selects owner perspective, and
   * `cursor` selects an index inside that area.
   */
  function createState() {
    return {
      cursor: 0,
      mode: "hand",
      player: 1,
      compact: false,
    };
  }

  function move(state, max, delta) {
    state.cursor = max > 0 ? (state.cursor + delta + max) % max : 0;
  }

  window.OpenJoeyDuelState = { createState, move };
})();
