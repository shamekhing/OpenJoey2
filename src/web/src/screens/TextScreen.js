(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};

  class TextScreen {
    constructor(app, name) {
      this.app = app;
      this.name = name;
    }

    key(event) {
      if (event.key === "Escape" || event.key === "Enter") {
        event.preventDefault();
        this.app.goto("menu");
      }
    }

    draw(g) {
      const title = this.name === "settings" ? "Settings" : "Testing";
      this.app.drawChrome(title, "ESC/ENTER back");
      const x = this.app.w < 520 ? 24 : 48;
      const y = this.app.chromeTop() + (this.app.w < 520 ? 56 : 64);
      g.fillStyle = "#f1f5f8";
      g.font = this.app.w < 520 ? "700 24px system-ui" : "700 28px system-ui";
      g.fillText(title, x, y, this.app.w - x * 2);
      g.fillStyle = "#9faab7";
      g.font = this.app.w < 520 ? "14px system-ui" : "16px system-ui";
      g.fillText("Screen shell is wired in src2. Add settings/test widgets here.", x, y + 34, this.app.w - x * 2);
    }
  }

  window.OpenJoeyScreens.TextScreen = TextScreen;
})();
