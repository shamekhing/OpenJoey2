(function () {
  const DuelState = window.OpenJoeyDuelState;
  const Selectors = window.OpenJoeyDuelSelectors;

  function key(app, state, event, api) {
    const block = ["ArrowDown", "ArrowUp", "ArrowLeft", "ArrowRight", "Enter", "Escape", "Tab"];
    if (block.includes(event.key)) event.preventDefault();
    if (event.key === "Escape") app.goto("menu");
    else if (event.key === "Tab") {
      state.mode = state.mode === "hand" ? "monster" : state.mode === "monster" ? "spell" : "hand";
      state.cursor = 0;
    } else if (event.key === "ArrowRight" || event.key === "ArrowDown") api.move(1);
    else if (event.key === "ArrowLeft" || event.key === "ArrowUp") api.move(-1);
    else if (event.key === "Enter") activate(app, state);
    else if (event.key.toLowerCase() === "d") drawCard(app, state);
    else if (event.key.toLowerCase() === "g") toGrave(app, state);
    else if (event.key.toLowerCase() === "n") nextPhase(app);
  }

  function move(app, state, delta) {
    const max = state.mode === "hand" ? app.duel.hand[state.player].length : 5;
    DuelState.move(state, max, delta);
  }

  function activate(app, state) {
    if (state.mode !== "hand") return;
    app.status = app.duel.playHandAt(state.player, state.cursor)
      ? "Played card from hand"
      : "No open zone";
    state.cursor = 0;
  }

  function drawCard(app, state) {
    app.status = app.duel.draw(state.player) ? "Drew a card" : "Deck empty";
  }

  function toGrave(app, state) {
    if (state.mode !== "monster") return;
    app.status = app.duel.sendMonsterToGrave(state.player, state.cursor)
      ? "Sent monster to grave"
      : "No monster there";
  }

  function nextPhase(app) {
    const phase = app.duel.advancePhase();
    const player = app.duel.turnPlayer === 1 ? "your" : "opponent";
    app.status = `${player} ${Selectors.phaseName(phase)}`;
  }

  window.OpenJoeyDuelActions = { key, move, activate, drawCard, toGrave, nextPhase };
})();
