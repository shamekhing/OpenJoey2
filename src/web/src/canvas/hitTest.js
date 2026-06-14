(function () {
  /**
   * Shared hit-testing helpers for canvas layout rectangles.
   */
  function pointInRect(rect, x, y) {
    return x >= rect.x && x <= rect.x + rect.w && y >= rect.y && y <= rect.y + rect.h;
  }

  function firstRect(rects, x, y) {
    return rects.findIndex((rect) => pointInRect(rect, x, y));
  }

  window.OpenJoeyHitTest = { pointInRect, firstRect };
})();
