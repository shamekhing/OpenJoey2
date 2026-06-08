(function () {
  class App {
    constructor(canvas, searchInput, cardDb, deck, duel, backend) {
      this.canvas = canvas;
      this.ctx = canvas.getContext("2d", { alpha: false });
      this.searchInput = searchInput;
      this.cardDb = cardDb;
      this.deck = deck;
      this.duel = duel;
      this.backend = backend;
      this.images = new window.OpenJoeyImageCache.ImageCache(140);
      this.status = `Ready (${backend})`;
      this.screen = null;
      this.resize();
      this.goto("menu");
      this.bind();
      requestAnimationFrame(() => this.frame());
    }

    bind() {
      window.addEventListener("resize", () => this.resize());
      document.addEventListener("keydown", (event) => this.screen?.key?.(event));
      this.canvas.addEventListener("click", (event) => {
        const rect = this.canvas.getBoundingClientRect();
        const x = event.clientX - rect.left;
        const y = event.clientY - rect.top;
        this.screen?.click?.(x, y);
      });
      this.canvas.addEventListener("dblclick", () => this.screen?.doubleClick?.());
    }

    resize() {
      const dpr = Math.min(window.devicePixelRatio || 1, 2);
      this.canvas.width = Math.floor(window.innerWidth * dpr);
      this.canvas.height = Math.floor(window.innerHeight * dpr);
      this.ctx.setTransform(dpr, 0, 0, dpr, 0, 0);
      this.w = window.innerWidth;
      this.h = window.innerHeight;
      this.screen?.layout?.();
    }

    chromeTop() {
      if (this.h < 430 || this.w < 430) return 48;
      return 56;
    }

    chromeBottom() {
      if (this.h < 430 || this.w < 430) return 32;
      return 40;
    }

    goto(name) {
      this.screen?.dispose?.();
      this.searchInput.style.display = "none";
      if (name === "menu") this.screen = new window.OpenJoeyScreens.MainMenuScreen(this);
      else if (name === "deck") this.screen = new window.OpenJoeyScreens.DeckEditorScreen(this);
      else if (name === "duel") this.screen = new window.OpenJoeyScreens.DuelScreen(this);
      else this.screen = new window.OpenJoeyScreens.TextScreen(this, name);
      this.screen.layout?.();
    }

    drawChrome(title, help) {
      const g = this.ctx;
      const topH = this.chromeTop();
      const bottomH = this.chromeBottom();
      g.fillStyle = "#0a0d11";
      g.fillRect(0, 0, this.w, this.h);
      g.fillStyle = "#11161c";
      g.fillRect(0, 0, this.w, topH);
      g.fillRect(0, this.h - bottomH, this.w, bottomH);
      g.strokeStyle = "#303946";
      g.beginPath();
      g.moveTo(0, topH - 0.5);
      g.lineTo(this.w, topH - 0.5);
      g.moveTo(0, this.h - bottomH + 0.5);
      g.lineTo(this.w, this.h - bottomH + 0.5);
      g.stroke();
      g.fillStyle = "#f1f5f8";
      g.font = topH < 52 ? "700 18px system-ui" : "700 21px system-ui";
      g.fillText(title, 12, topH < 52 ? 31 : 36, Math.max(110, this.w * 0.48));
      g.fillStyle = "#9faab7";
      g.font = topH < 52 ? "12px system-ui" : "14px system-ui";
      g.textAlign = "right";
      g.fillText(this.status, this.w - 12, topH < 52 ? 30 : 35, Math.max(72, this.w * 0.42));
      g.textAlign = "left";
      g.font = bottomH < 36 ? "11px system-ui" : "12px system-ui";
      g.fillText(help, 12, this.h - Math.max(11, Math.floor(bottomH * 0.4)), this.w - 24);
    }

    frame() {
      this.screen?.draw?.(this.ctx);
      requestAnimationFrame(() => this.frame());
    }
  }

  window.OpenJoeyApp = { App };
})();
