(function () {
  /**
   * Shared chrome metrics and drawing for all screens.
   */
  function top(app) {
    if (app.h < 430 || app.w < 430) return 48;
    return 56;
  }

  function bottom(app) {
    if (app.h < 430 || app.w < 430) return 32;
    return 40;
  }

  function draw(app, title, help) {
    const g = app.ctx;
    const topH = top(app);
    const bottomH = bottom(app);
    g.fillStyle = "#0a0d11";
    g.fillRect(0, 0, app.w, app.h);
    g.fillStyle = "#11161c";
    g.fillRect(0, 0, app.w, topH);
    g.fillRect(0, app.h - bottomH, app.w, bottomH);
    g.strokeStyle = "#303946";
    g.beginPath();
    g.moveTo(0, topH - 0.5);
    g.lineTo(app.w, topH - 0.5);
    g.moveTo(0, app.h - bottomH + 0.5);
    g.lineTo(app.w, app.h - bottomH + 0.5);
    g.stroke();
    g.fillStyle = "#f1f5f8";
    g.font = topH < 52 ? "700 18px system-ui" : "700 21px system-ui";
    g.fillText(title, 12, topH < 52 ? 31 : 36, Math.max(110, app.w * 0.48));
    g.fillStyle = "#9faab7";
    g.font = topH < 52 ? "12px system-ui" : "14px system-ui";
    g.textAlign = "right";
    g.fillText(app.status, app.w - 12, topH < 52 ? 30 : 35, Math.max(72, app.w * 0.42));
    g.textAlign = "left";
    g.font = bottomH < 36 ? "11px system-ui" : "12px system-ui";
    g.fillText(help, 12, app.h - Math.max(11, Math.floor(bottomH * 0.4)), app.w - 24);
  }

  window.OpenJoeyChromeLayout = { top, bottom, draw };
})();
