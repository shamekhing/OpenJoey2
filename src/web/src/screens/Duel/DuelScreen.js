(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Ui = window.OpenJoeyUi;
  const Card = window.OpenJoeyCardDb;
  const { CARD_ASPECT } = window.OpenJoeyDeckConstants;
  const Actions = window.OpenJoeyDuelActions;
  const Selectors = window.OpenJoeyDuelSelectors;
  const View = window.OpenJoeyDuelView;

  /**
   * Duel screen.
   *
   * This screen still owns most duel layout and drawing code directly. Keep
   * helper methods grouped by responsibility so they can move into layout/view
   * modules cleanly as the duel UI grows.
   */
  class DuelScreen {
    constructor(app) {
      this.app = app;
      this.state = window.OpenJoeyDuelState.createState();
      this.startFromDeck();
    }

    get cursor() {
      return this.state.cursor;
    }

    set cursor(value) {
      this.state.cursor = value;
    }

    get mode() {
      return this.state.mode;
    }

    set mode(value) {
      this.state.mode = value;
    }

    get player() {
      return this.state.player;
    }

    set player(value) {
      this.state.player = value;
    }

    startFromDeck() {
      // Player 1 is the local user; player 0 is a simple generated opponent.
      const own = this.app.deck.cards.length
        ? this.app.deck.cards
        : [...this.app.cardDb.cards].slice(0, 40);
      const opp = [...this.app.cardDb.cards].slice(40, 80);
      this.app.duel.loadDeck(opp, own);
      this.app.duel.start();
      this.app.status = `Duel started (${this.app.backend})`;
    }

    key(event) {
      Actions.key(this.app, this.state, event, { move: (delta) => this.move(delta) });
    }

    move(delta) {
      Actions.move(this.app, this.state, delta);
    }

    activate() {
      Actions.activate(this.app, this.state);
    }

    drawCard() {
      Actions.drawCard(this.app, this.state);
    }

    toGrave() {
      Actions.toGrave(this.app, this.state);
    }

    nextPhase() {
      Actions.nextPhase(this.app);
    }

    click(x, y) {
      const ownHand = this.hitHand(this.ownHand, this.app.duel.hand[1], x, y);
      if (ownHand >= 0) {
        this.mode = "hand";
        this.player = 1;
        this.cursor = ownHand;
        return;
      }
      const ownMonster = this.hitZones(this.ownMonsters, x, y);
      if (ownMonster >= 0) {
        this.mode = "monster";
        this.player = 1;
        this.cursor = ownMonster;
        return;
      }
      const ownSpell = this.hitZones(this.ownSpells, x, y);
      if (ownSpell >= 0) {
        this.mode = "spell";
        this.player = 1;
        this.cursor = ownSpell;
        return;
      }
      const oppMonster = this.hitZones(this.oppMonsters, x, y);
      if (oppMonster >= 0) {
        this.mode = "monster";
        this.player = 0;
        this.cursor = oppMonster;
        return;
      }
      const oppSpell = this.hitZones(this.oppSpells, x, y);
      if (oppSpell >= 0) {
        this.mode = "spell";
        this.player = 0;
        this.cursor = oppSpell;
      }
    }

    doubleClick() {
      if (this.player === 1) this.activate();
    }

    draw(g) {
      View.draw(this, g);
    }

    drawChromeAndBoard(g) {
      this.app.drawChrome(
        "DUEL",
        "[TAB] hand/monster/spell [ENTER] play own hand [N] next phase [D] draw own card [G] own monster to grave [ESC] menu",
      );
      this.layout();
      this.drawField(g);
      this.drawHands(g);
      this.drawPreview(g);
    }

    layout() {
      const compact = this.app.w < 900 || this.app.h < 520;
      this.compact = compact;
      const sideW = compact ? 0 : Math.max(238, Math.min(310, Math.floor(this.app.w * 0.22)));
      const outer = 14;
      const top = this.app.chromeTop() + (compact ? 6 : 12);
      const bottomBar = this.app.chromeBottom() + (compact ? 8 : 14);
      const handH = compact ? Math.max(66, Math.min(86, Math.floor(this.app.h * 0.22))) : 88;
      const gap = compact ? 6 : 14;

      this.preview = {
        x: outer,
        y: compact ? 0 : top,
        w: compact ? 0 : sideW,
        h: compact ? 0 : this.app.h - top - bottomBar - gap,
      };

      const fieldX = compact ? outer : outer + sideW + gap;
      const fieldW = compact ? this.app.w - outer * 2 : this.app.w - fieldX - outer;
      this.field = {
        x: fieldX,
        y: top,
        w: fieldW,
        h: Math.max(120, this.app.h - top - bottomBar - (compact ? handH + gap : handH * 2 + gap * 2)),
      };

      this.oppHand = compact
        ? { x: fieldX, y: 0, w: fieldW, h: 0 }
        : { x: fieldX, y: top + this.field.h + gap, w: fieldW, h: handH };
      this.ownHand = compact
        ? { x: fieldX, y: this.field.y + this.field.h + gap, w: fieldW, h: handH }
        : { x: fieldX, y: this.oppHand.y + handH + gap, w: fieldW, h: handH };

      const pileW = compact ? 40 : Math.max(46, Math.min(70, Math.floor(this.field.w * 0.08)));
      // Zone rows are centered between deck/GY piles so the mat remains stable
      // across compact and desktop layouts.
      const playableW = this.field.w - pileW * 2 - gap * 4;
      const zoneGap = Math.max(6, Math.min(12, Math.floor(playableW * 0.016)));
      const topInset = compact ? 40 : 44;
      const bottomInset = compact ? 12 : 16;
      let zoneW;
      let zoneH;
      let spellH;
      if (compact) {
        spellH = 22;
        zoneH = Math.max(34, Math.min(64, (this.field.h - topInset - bottomInset - spellH * 2 - gap * 3) / 2));
        zoneW = Math.max(30, Math.min(48, zoneH / CARD_ASPECT, (playableW - zoneGap * 4) / 5));
      } else {
        const maxZoneByWidth = (playableW - zoneGap * 4) / 5;
        const maxZoneByHeight = ((this.field.h - topInset - bottomInset - gap * 3) / 4) / CARD_ASPECT;
        zoneW = Math.max(30, Math.min(84, maxZoneByWidth, maxZoneByHeight));
        zoneH = zoneW * CARD_ASPECT;
        spellH = zoneH;
      }
      const startX = this.field.x + (this.field.w - (zoneW * 5 + zoneGap * 4)) / 2;
      const rowsH = compact ? spellH * 2 + zoneH * 2 + gap * 3 : zoneH * 4 + gap * 3;
      const startY = compact
        ? this.field.y + topInset
        : this.field.y + Math.max(topInset, (this.field.h - rowsH) / 2);

      this.oppSpells = this.zones(startX, startY, zoneW, spellH, zoneGap);
      this.oppMonsters = this.zones(startX, startY + spellH + gap, zoneW, zoneH, zoneGap);
      this.ownMonsters = this.zones(startX, startY + spellH + zoneH + gap * 2, zoneW, zoneH, zoneGap);
      this.ownSpells = this.zones(startX, startY + spellH + zoneH * 2 + gap * 3, zoneW, spellH, zoneGap);

      const pileH = compact ? 38 : zoneH;
      const leftPileX = this.field.x + gap;
      const rightPileX = this.field.x + this.field.w - gap - pileW;
      if (compact) {
        // Compact mode folds opponent hand away and stacks piles near edges.
        const topPileY = this.field.y + 42;
        const ownPileY = this.field.y + this.field.h - pileH - 12;
        this.piles = {
          oppDeck: { x: rightPileX, y: topPileY, w: pileW, h: pileH },
          oppGrave: { x: leftPileX, y: topPileY, w: pileW, h: pileH },
          oppBanished: { x: leftPileX, y: topPileY + pileH + 6, w: pileW, h: pileH },
          ownBanished: { x: rightPileX, y: ownPileY - pileH - 6, w: pileW, h: pileH },
          ownGrave: { x: leftPileX, y: ownPileY, w: pileW, h: pileH },
          ownDeck: { x: rightPileX, y: ownPileY, w: pileW, h: pileH },
        };
      } else {
        this.piles = {
          oppDeck: { x: rightPileX, y: this.oppSpells[0].y, w: pileW, h: pileH },
          oppGrave: { x: leftPileX, y: this.oppSpells[0].y, w: pileW, h: pileH },
          oppBanished: { x: leftPileX, y: this.oppMonsters[0].y, w: pileW, h: pileH },
          ownBanished: { x: rightPileX, y: this.ownMonsters[0].y, w: pileW, h: pileH },
          ownGrave: { x: leftPileX, y: this.ownSpells[0].y, w: pileW, h: pileH },
          ownDeck: { x: rightPileX, y: this.ownSpells[0].y, w: pileW, h: pileH },
        };
      }
    }

    zones(startX, y, w, h, gap) {
      return Array.from({ length: 5 }, (_, i) => ({ x: startX + i * (w + gap), y, w, h }));
    }

    selectedCard() {
      return Selectors.selectedCard(this.app, this.state);
    }

    drawPreview(g) {
      if (this.preview.w <= 0 || this.preview.h <= 0) return;
      Ui.roundRect(g, this.preview, 8, "rgba(13,17,22,.94)", "#3b4652");
      const card = this.selectedCard();
      g.fillStyle = "#f1f5f8";
      g.font = "700 16px system-ui";
      g.fillText("Selected", this.preview.x + 14, this.preview.y + 27);
      const compact = this.preview.h < 130;
      const w = compact ? 50 : Math.min(this.preview.w - 50, 212);
      const h = w * CARD_ASPECT;
      const x = compact ? this.preview.x + 14 : this.preview.x + (this.preview.w - w) / 2;
      const y = compact ? this.preview.y + 38 : this.preview.y + 54;
      Ui.cardImage(g, this.app.images, card, x, y, w, h);
      if (!card) return;
      const textX = compact ? x + w + 12 : this.preview.x + 16;
      const textY = compact ? this.preview.y + 52 : y + h + 30;
      const textW = compact ? this.preview.w - (textX - this.preview.x) - 14 : this.preview.w - 32;
      g.fillStyle = "#f1f5f8";
      g.font = "700 17px system-ui";
      Ui.text(g, card.name, textX, textY, textW);
      g.fillStyle = Ui.kindColor(card.kind);
      g.font = "14px system-ui";
      g.fillText(Card.kindTag(card.kind), textX, textY + 24);
      g.fillStyle = "#d8e0e8";
      if (compact) {
        g.font = "12px system-ui";
        g.fillText(card.kind === 0 ? Card.statsLine(card, true) : Card.kindName(card.kind), textX, textY + 46);
      } else {
        Ui.wrap(g, card.desc, this.preview.x + 16, y + h + 82, this.preview.w - 32, 18, 11);
      }
    }

    drawField(g) {
      this.drawMat(g);
      this.drawPlayerRail(g, 0);
      this.drawPlayerRail(g, 1);
      this.drawPhaseBadge(g);
      if (!this.compact) {
        this.drawRowLabel(g, "SPELL & TRAP", this.oppSpells, false);
        this.drawRowLabel(g, "MONSTER ZONE", this.oppMonsters, false);
        this.drawRowLabel(g, "MONSTER ZONE", this.ownMonsters, true);
        this.drawRowLabel(g, "SPELL & TRAP", this.ownSpells, true);
      }
      this.drawPiles(g);
      for (let i = 0; i < 5; i += 1) {
        this.drawZone(g, this.oppSpells[i], this.app.duel.spells[0][i], this.player === 0 && this.mode === "spell" && this.cursor === i, "spell");
        this.drawZone(g, this.oppMonsters[i], this.app.duel.monsters[0][i], this.player === 0 && this.mode === "monster" && this.cursor === i, "monster");
        this.drawZone(g, this.ownMonsters[i], this.app.duel.monsters[1][i], this.player === 1 && this.mode === "monster" && this.cursor === i, "monster");
        this.drawZone(g, this.ownSpells[i], this.app.duel.spells[1][i], this.player === 1 && this.mode === "spell" && this.cursor === i, "spell");
      }
    }

    drawMat(g) {
      const r = this.field;
      const grd = g.createLinearGradient(r.x, r.y, r.x + r.w, r.y + r.h);
      grd.addColorStop(0, "#12251f");
      grd.addColorStop(0.45, "#17232a");
      grd.addColorStop(1, "#2a1d27");
      Ui.roundRect(g, r, 8, grd, "#41505b");
      g.fillStyle = "rgba(235,220,161,.08)";
      g.fillRect(r.x + 12, r.y + r.h / 2 - 1, r.w - 24, 2);
      g.strokeStyle = "rgba(243,212,91,.25)";
      g.beginPath();
      g.moveTo(r.x + r.w / 2, r.y + 12);
      g.lineTo(r.x + r.w / 2, r.y + r.h - 12);
      g.stroke();
    }

    drawPlayerRail(g, player) {
      const topSide = player === 0;
      const d = this.app.duel;
      const inset = this.compact ? 66 : 14;
      const rail = {
        x: this.field.x + inset,
        y: topSide ? this.field.y + 12 : this.field.y + this.field.h - 38,
        w: this.field.w - inset * 2,
        h: 28,
      };
      g.fillStyle = topSide ? "rgba(132,92,196,.22)" : "rgba(67,169,117,.22)";
      g.fillRect(rail.x, rail.y, rail.w, rail.h);
      g.strokeStyle = topSide ? "rgba(174,139,225,.42)" : "rgba(103,211,146,.42)";
      g.strokeRect(rail.x + 0.5, rail.y + 0.5, rail.w - 1, rail.h - 1);
      g.fillStyle = "#f1f5f8";
      g.font = "700 13px system-ui";
      g.fillText(this.compact && player === 0 ? "OPP" : player === 0 ? "OPPONENT" : "YOU", rail.x + 10, rail.y + 19);
      if (d.turnPlayer === player) {
        g.fillStyle = topSide ? "#d8c5ff" : "#b4f0ca";
        g.fillText("TURN", rail.x + (this.compact ? 46 : 92), rail.y + 19);
      }
      g.textAlign = "right";
      g.fillText(`LP ${d.lp[player]}`, rail.x + rail.w - 10, rail.y + 19);
      g.textAlign = "left";
    }

    drawPhaseBadge(g) {
      const names = ["DRAW", "STANDBY", "MAIN 1", "BATTLE", "MAIN 2", "END"];
      const text = names[this.app.duel.phase] || `PHASE ${this.app.duel.phase}`;
      const w = this.compact ? 74 : Math.min(150, Math.max(104, this.field.w * 0.18));
      const r = {
        x: this.field.x + (this.field.w - w) / 2,
        y: this.compact ? this.field.y + 8 : this.field.y + this.field.h / 2 - 15,
        w,
        h: this.compact ? 22 : 30,
      };
      Ui.roundRect(g, r, 6, "rgba(8,11,14,.78)", "#e0b854");
      g.fillStyle = "#f2d46f";
      g.font = this.compact ? "700 10px system-ui" : "700 12px system-ui";
      g.textAlign = "center";
      g.fillText(text, r.x + r.w / 2, r.y + (this.compact ? 15 : 20));
      g.textAlign = "left";
    }

    drawRowLabel(g, text, zones, own) {
      const first = zones[0];
      const last = zones[zones.length - 1];
      g.fillStyle = own ? "rgba(103,211,146,.9)" : "rgba(174,139,225,.9)";
      g.font = "700 10px system-ui";
      g.textAlign = "center";
      g.fillText(text, first.x + (last.x + last.w - first.x) / 2, first.y - 7);
      g.textAlign = "left";
    }

    drawPiles(g) {
      const d = this.app.duel;
      this.drawPile(g, this.piles.oppDeck, "DECK", d.deck[0].length, true);
      this.drawPile(g, this.piles.oppGrave, "GY", d.graveCount?.[0] ?? d.grave[0].length, false);
      this.drawPile(g, this.piles.oppBanished, "BAN", d.banishedCount?.[0] ?? d.banished[0].length, false);
      this.drawPile(g, this.piles.ownDeck, "DECK", d.deck[1].length, true);
      this.drawPile(g, this.piles.ownGrave, "GY", d.graveCount?.[1] ?? d.grave[1].length, false);
      this.drawPile(g, this.piles.ownBanished, "BAN", d.banishedCount?.[1] ?? d.banished[1].length, false);
    }

    drawPile(g, rect, label, count, deck) {
      Ui.roundRect(g, rect, 6, deck ? "rgba(70,43,30,.88)" : "rgba(15,20,25,.78)", deck ? "#c89b63" : "#596673");
      g.fillStyle = deck ? "rgba(216,174,103,.18)" : "rgba(214,224,232,.08)";
      g.fillRect(rect.x + 5, rect.y + 6, rect.w - 10, rect.h - 12);
      g.fillStyle = "#dce5ed";
      g.font = this.compact ? "700 9px system-ui" : "700 10px system-ui";
      g.textAlign = "center";
      g.fillText(label, rect.x + rect.w / 2, rect.y + rect.h / 2 - 2);
      g.font = this.compact ? "11px system-ui" : "12px system-ui";
      g.fillText(String(count), rect.x + rect.w / 2, rect.y + rect.h / 2 + 15);
      g.textAlign = "left";
    }

    drawZone(g, rect, card, selected, type) {
      const stroke = selected ? "#f3d45b" : type === "monster" ? "#b65e64" : "#62b979";
      const fill = type === "monster" ? "rgba(55,27,31,.86)" : "rgba(25,52,38,.84)";
      Ui.roundRect(g, rect, 6, fill, stroke);
      g.fillStyle = "rgba(255,255,255,.045)";
      g.fillRect(rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10);
      if (selected) {
        g.strokeStyle = "#f3d45b";
        g.lineWidth = 2;
        g.strokeRect(rect.x - 3, rect.y - 3, rect.w + 6, rect.h + 6);
        g.lineWidth = 1;
      }
      if (card) Ui.cardImage(g, this.app.images, card, rect.x + 5, rect.y + 5, rect.w - 10, rect.h - 10);
    }

    drawHands(g) {
      if (!this.compact) this.drawHand(g, this.oppHand, this.app.duel.hand[0], false);
      this.drawHand(g, this.ownHand, this.app.duel.hand[1], true);
    }

    drawHand(g, rect, cards, own) {
      Ui.roundRect(g, rect, 8, own ? "rgba(13,22,18,.94)" : "rgba(16,15,24,.94)", own && this.mode === "hand" && this.player === 1 ? "#f3d45b" : "#3a4651");
      g.fillStyle = own ? "#67d392" : "#ae8be1";
      g.font = "700 11px system-ui";
      g.fillText(own ? "YOUR HAND" : "OPPONENT HAND", rect.x + 12, rect.y + 18);
      const maxCards = Math.max(1, cards.length);
      const maxByHeight = Math.max(22, (rect.h - 24) / CARD_ASPECT);
      const w = Math.max(22, Math.min(maxByHeight, 50, (rect.w - 34) / Math.max(8, maxCards)));
      const h = w * CARD_ASPECT;
      const overlap = cards.length > 1 ? Math.min(w + 8, (rect.w - w - 24) / (cards.length - 1)) : 0;
      const startX = rect.x + 12 + Math.max(0, (rect.w - 24 - (w + overlap * (cards.length - 1))) / 2);
      const y = rect.y + rect.h - h - 8;
      for (let i = 0; i < cards.length; i += 1) {
        const x = startX + i * overlap;
        if (own && this.mode === "hand" && this.cursor === i) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(x - 3, y - 3, w + 6, h + 6);
        }
        own ? Ui.cardImage(g, this.app.images, cards[i], x, y, w, h)
            : Ui.cardImage(g, this.app.images, null, x, y, w, h);
      }
    }

    hitZones(zones, x, y) {
      return zones.findIndex((r) => x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
    }

    hitHand(rect, cards, x, y) {
      if (x < rect.x || x > rect.x + rect.w || y < rect.y || y > rect.y + rect.h) return -1;
      if (!cards.length) return -1;
      const maxCards = Math.max(1, cards.length);
      const maxByHeight = Math.max(22, (rect.h - 24) / CARD_ASPECT);
      const w = Math.max(22, Math.min(maxByHeight, 50, (rect.w - 34) / Math.max(8, maxCards)));
      const overlap = cards.length > 1 ? Math.min(w + 8, (rect.w - w - 24) / (cards.length - 1)) : 0;
      const startX = rect.x + 12 + Math.max(0, (rect.w - 24 - (w + overlap * (cards.length - 1))) / 2);
      // Walk backwards so overlapping cards select the topmost visible card.
      for (let i = cards.length - 1; i >= 0; i -= 1) {
        const cx = startX + i * overlap;
        const cy = rect.y + rect.h - (w * CARD_ASPECT) - 8;
        if (x >= cx && x <= cx + w && y >= cy && y <= cy + w * CARD_ASPECT) return i;
      }
      return -1;
    }

  }

  window.OpenJoeyScreens.DuelScreen = DuelScreen;
})();
