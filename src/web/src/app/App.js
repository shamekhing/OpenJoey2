(function () {
  const { CanvasRenderer } = window.OpenJoeyCanvasRenderer;
  const { Input } = window.OpenJoeyInput;
  const { Router } = window.OpenJoeyRouter;
  const Chrome = window.OpenJoeyChromeLayout;
  const { Store } = window.OpenJoeyStore;

  /**
   * Owns process-wide browser state: renderer, input, route, card database,
   * and the active deck/duel backend.
   */
  class App {
    constructor(canvas, searchInput, cardDb, deck, duel, backend) {
      this.canvas = canvas;
      this.searchInput = searchInput;
      this.cardDb = cardDb;
      this.deck = deck;
      this.duel = duel;
      this.backend = backend;
      this.images = new window.OpenJoeyImageCache.ImageCache(140);
      this.store = new Store({ route: "menu" });
      this.status = `Ready (${backend})`;
      this.renderer = new CanvasRenderer(canvas, this);
      this.ctx = this.renderer.ctx;
      this.router = new Router(this);
      this.input = new Input(this);
      this.resize();
      this.goto("menu");
      this.input.bind();
      this.renderer.start();
    }

    resize() {
      const size = this.renderer.resize();
      this.w = size.w;
      this.h = size.h;
      // Screens cache layout rectangles, so recompute after every canvas resize.
      this.screen?.layout?.();
    }

    chromeTop() {
      return Chrome.top(this);
    }

    chromeBottom() {
      return Chrome.bottom(this);
    }

    goto(name) {
      this.store.set({ route: name });
      this.screen = this.router.goto(name);
    }

    drawChrome(title, help) {
      Chrome.draw(this, title, help);
    }

    replaceCardDb(rows, sourceLabel) {
      const oldDeckIds = this.deck.cards.map((card) => card.id);
      this.cardDb = new window.OpenJoeyCardDb.CardDb(rows);
      this.cardDb.rebuildSort(0);
      this.deck.clear();
      let restored = 0;
      for (const id of oldDeckIds) {
        const card = this.cardDb.byId.get(id);
        if (card && this.deck.add(card)) restored += 1;
      }
      window.OpenJoeyDeckStorage.save(this.deck);
      window.OpenJoeyCardDbCache?.saveRows?.(rows);
      this.status = `${sourceLabel}: ${this.cardDb.cards.length} cards, ${restored} deck cards kept`;
    }
  }

  window.OpenJoeyApp = { App };
})();
