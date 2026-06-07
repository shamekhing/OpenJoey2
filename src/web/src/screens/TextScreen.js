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
      g.fillStyle = "#f1f5f8";
      g.font = "700 28px system-ui";
      g.fillText(title, 48, 120);
      g.fillStyle = "#9faab7";
      g.font = "16px system-ui";
      g.fillText("Screen shell is wired in src2. Add settings/test widgets here.", 48, 154);
    }
  }

  window.OpenJoeyScreens.TextScreen = TextScreen;
})();
