(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};

  class MainMenuScreen {
    constructor(app) {
      this.app = app;
      this.items = [
        ["Duel", "duel"],
        ["Deck Editor", "deck"],
        ["Settings", "settings"],
        ["Testing", "testing"],
        ["Quit", "quit"],
      ];
      this.cursor = 0;
    }

    key(event) {
      if (event.key === "ArrowDown") {
        event.preventDefault();
        this.cursor = (this.cursor + 1) % this.items.length;
      } else if (event.key === "ArrowUp") {
        event.preventDefault();
        this.cursor = (this.cursor + this.items.length - 1) % this.items.length;
      } else if (event.key === "Enter" || event.key === " ") {
        event.preventDefault();
        const target = this.items[this.cursor][1];
        if (target === "quit") this.app.status = "Quit is native-only in browser";
        else this.app.goto(target);
      }
    }

    click(x, y) {
      const metrics = this.metrics();
      const row = Math.floor((y - metrics.startY + metrics.rowH / 2) / metrics.rowH);
      if (row >= 0 && row < this.items.length) {
        this.cursor = row;
        const target = this.items[row][1];
        if (target !== "quit") this.app.goto(target);
      }
    }

    metrics() {
      const short = this.app.h < 460;
      const titleSize = Math.max(34, Math.min(short ? 44 : 64, Math.floor(this.app.w * 0.15)));
      const itemSize = Math.max(20, Math.min(short ? 24 : 28, Math.floor(this.app.w * 0.072)));
      const rowH = Math.max(34, itemSize + 14);
      const top = this.app.chromeTop();
      const bottom = this.app.chromeBottom();
      const titleY = Math.max(top + titleSize + 10, Math.floor(this.app.h * (short ? 0.27 : 0.25)));
      const menuH = rowH * this.items.length;
      const startY = Math.min(
        this.app.h - bottom - menuH + rowH * 0.78,
        Math.max(titleY + rowH * 1.35, Math.floor(this.app.h * (short ? 0.47 : 0.5))),
      );
      return { titleSize, itemSize, rowH, titleY, startY };
    }

    draw(g) {
      this.app.drawChrome("OpenJoey", "UP/DOWN to navigate, ENTER to select");
      const metrics = this.metrics();
      g.fillStyle = "#d7b84d";
      g.font = `700 ${metrics.titleSize}px system-ui`;
      g.textAlign = "center";
      g.fillText("OpenJoey", this.app.w / 2, metrics.titleY, this.app.w - 28);
      g.font = `${metrics.itemSize}px system-ui`;
      for (let i = 0; i < this.items.length; i += 1) {
        const y = metrics.startY + i * metrics.rowH;
        g.fillStyle = i === this.cursor ? "#f3d45b" : "#d7dde6";
        g.fillText(this.items[i][0], this.app.w / 2, y);
        if (i === this.cursor) {
          const markerX = Math.max(24, this.app.w / 2 - Math.min(150, this.app.w * 0.36));
          g.fillText(">", markerX, y);
        }
      }
      g.textAlign = "left";
    }
  }

  window.OpenJoeyScreens.MainMenuScreen = MainMenuScreen;
})();
