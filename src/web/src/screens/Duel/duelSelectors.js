(function () {
  function selectedCard(app, state) {
    if (state.mode === "hand") return app.duel.hand[state.player][state.cursor] || null;
    if (state.mode === "monster") return app.duel.monsters[state.player][state.cursor] || null;
    return app.duel.spells[state.player][state.cursor] || null;
  }

  function phaseName(phase) {
    return ["Draw", "Standby", "Main 1", "Battle", "Main 2", "End"][phase] || `phase ${phase}`;
  }

  window.OpenJoeyDuelSelectors = { selectedCard, phaseName };
})();
