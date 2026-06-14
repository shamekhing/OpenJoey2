(function () {
  class Input {
    constructor(app) {
      this.app = app;
    }

    bind() {
      window.addEventListener("resize", () => this.app.resize());
      document.addEventListener("keydown", (event) => this.app.screen?.key?.(event));
      this.app.canvas.addEventListener("click", (event) => {
        const point = this.canvasPoint(event);
        this.app.screen?.click?.(point.x, point.y);
      });
      this.app.canvas.addEventListener("dblclick", () => this.app.screen?.doubleClick?.());
    }

    canvasPoint(event) {
      const rect = this.app.canvas.getBoundingClientRect();
      return {
        x: event.clientX - rect.left,
        y: event.clientY - rect.top,
      };
    }
  }

  window.OpenJoeyInput = { Input };
})();
