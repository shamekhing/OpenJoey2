(function () {
  function rect(ctx, r, fill, stroke) {
    ctx.fillStyle = fill;
    ctx.fillRect(r.x, r.y, r.w, r.h);
    if (stroke) {
      ctx.strokeStyle = stroke;
      ctx.strokeRect(r.x + 0.5, r.y + 0.5, r.w - 1, r.h - 1);
    }
  }

  function text(ctx, value, x, y, maxWidth) {
    let s = String(value || "");
    while (s.length && ctx.measureText(s).width > maxWidth) s = s.slice(0, -1);
    if (s.length < String(value || "").length) s = `${s.slice(0, -1)}~`;
    ctx.fillText(s, x, y);
  }

  function wrap(ctx, value, x, y, maxWidth, lineH, maxLines) {
    const words = String(value || "").split(/\s+/);
    let line = "";
    let lines = 0;
    for (const word of words) {
      const test = line ? `${line} ${word}` : word;
      if (ctx.measureText(test).width > maxWidth && line) {
        ctx.fillText(line, x, y + lines * lineH);
        line = word;
        lines += 1;
        if (lines >= maxLines) return;
      } else {
        line = test;
      }
    }
    if (line && lines < maxLines) ctx.fillText(line, x, y + lines * lineH);
  }

  function visibleStart(total, cursor, max) {
    if (total <= max) return 0;
    return Math.max(0, Math.min(cursor - Math.floor(max / 2), total - max));
  }

  function cardImage(ctx, images, card, x, y, w, h) {
    if (!card) {
      ctx.fillStyle = "#5a2f22";
      ctx.fillRect(x, y, w, h);
      ctx.strokeStyle = "#c59d71";
      ctx.strokeRect(x, y, w, h);
      return;
    }
    const img = images.get(window.OpenJoeyCardDb.imageUrl(card));
    if (img) ctx.drawImage(img, x, y, w, h);
    else {
      ctx.fillStyle = card.kind === 1 ? "#163d2f" : card.kind === 2 ? "#48233a" : "#4b2f20";
      ctx.fillRect(x, y, w, h);
    }
    ctx.strokeStyle = "#303946";
    ctx.strokeRect(x, y, w, h);
  }

  function kindColor(kind) {
    if (kind === 1) return "#56c978";
    if (kind === 2) return "#d86fac";
    return "#d06062";
  }

  window.OpenJoeyUi = { rect, text, wrap, visibleStart, cardImage, kindColor };
})();
