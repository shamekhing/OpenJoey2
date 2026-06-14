(function () {
  /**
   * Small LRU-ish image cache for remote card art.
   *
   * `get()` returns null until the image has loaded, letting the renderer draw
   * a deterministic placeholder without blocking the frame.
   */
  class ImageCache {
    constructor(limit = 96) {
      this.limit = limit;
      this.map = new Map();
    }

    get(url) {
      let entry = this.map.get(url);
      if (entry) {
        entry.used = performance.now();
        return entry.image.complete && entry.image.naturalWidth ? entry.image : null;
      }

      const image = new Image();
      image.decoding = "async";
      image.src = url;
      entry = { image, used: performance.now() };
      this.map.set(url, entry);
      this.evict();
      return null;
    }

    evict() {
      if (this.map.size <= this.limit) return;
      // Drop least-recently-used entries first.
      const entries = [...this.map.entries()].sort((a, b) => a[1].used - b[1].used);
      for (let i = 0; i < entries.length && this.map.size > this.limit; i += 1) {
        this.map.delete(entries[i][0]);
      }
    }
  }

  /**
   * Owns the canvas context and animation frame lifecycle.
   */
  class CanvasRenderer {
    constructor(canvas, app) {
      this.canvas = canvas;
      this.app = app;
      this.ctx = canvas.getContext("2d", { alpha: false });
    }

    resize() {
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      // Cap DPR so card-heavy screens stay cheap on very dense displays.
      this.canvas.width = Math.floor(window.innerWidth * dpr);
      this.canvas.height = Math.floor(window.innerHeight * dpr);
      this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      return { w: window.innerWidth, h: window.innerHeight };
    }

    start() {
      requestAnimationFrame(() => this.frame());
    }

    frame() {
      this.app.screen?.draw?.(this.ctx);
      requestAnimationFrame(() => this.frame());
    }
  }

  window.OpenJoeyCanvasRenderer = { CanvasRenderer };
  window.OpenJoeyImageCache = { ImageCache };
})();
