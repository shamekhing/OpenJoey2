(function () {
  /**
   * Thin view adapter for the duel screen.
   *
   * Kept as a separate module so the remaining draw helpers can move here
   * without changing Router/App contracts.
   */
  function draw(screen, g) {
    screen.drawChromeAndBoard(g);
  }

  window.OpenJoeyDuelView = { draw };
})();
