(function () {
  const { CARD_ASPECT } = window.OpenJoeyDeckConstants;

  function cardHeight(width) {
    return width * CARD_ASPECT;
  }

  window.OpenJoeyDuelLayout = { cardHeight };
})();
