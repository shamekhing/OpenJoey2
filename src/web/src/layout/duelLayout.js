(function () {
  const { CARD_ASPECT } = window.OpenJoeyDeckConstants;

  /**
   * Duel layout helpers shared by future duel view extractions.
   */
  function cardHeight(width) {
    return width * CARD_ASPECT;
  }

  window.OpenJoeyDuelLayout = { cardHeight };
})();
