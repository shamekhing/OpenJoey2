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
      const startY = this.app.h / 2;
      const row = Math.floor((y - startY + 8) / 44);
      if (row >= 0 && row < this.items.length) {
        this.cursor = row;
        const target = this.items[row][1];
        if (target !== "quit") this.app.goto(target);
      }
    }

    draw(g) {
      this.app.drawChrome("OpenJoey", "UP/DOWN to navigate, ENTER to select");
      g.fillStyle = "#d7b84d";
      g.font = "700 64px system-ui";
      g.textAlign = "center";
      g.fillText("OpenJoey", this.app.w / 2, this.app.h / 4);
      const startY = this.app.h / 2;
      g.font = "28px system-ui";
      for (let i = 0; i < this.items.length; i += 1) {
        g.fillStyle = i === this.cursor ? "#f3d45b" : "#d7dde6";
        g.fillText(this.items[i][0], this.app.w / 2, startY + i * 44);
        if (i === this.cursor) g.fillText(">", this.app.w / 2 - 150, startY + i * 44);
      }
      g.textAlign = "left";
    }
  }

  window.OpenJoeyScreens.MainMenuScreen = MainMenuScreen;
})();
