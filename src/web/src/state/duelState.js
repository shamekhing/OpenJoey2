(function () {
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
