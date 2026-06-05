(function () {
  window.OpenJoeyScreens = window.OpenJoeyScreens || {};
  const Ui = window.OpenJoeyUi;
  const Card = window.OpenJoeyCardDb;

  class DuelScreen {
    constructor(app) {
      this.app = app;
      this.cursor = 0;
      this.mode = "hand";
      this.player = 1;
      this.startFromDeck();
    }

    startFromDeck() {
      const own = this.app.deck.cards.length
        ? this.app.deck.cards
        : [...this.app.cardDb.cards].slice(0, 40);
      const opp = [...this.app.cardDb.cards].slice(40, 80);
      this.app.duel.loadDeck(opp, own);
      this.app.duel.start();
      this.app.status = `Duel started (${this.app.backend})`;
    }

    key(event) {
      const block = ["ArrowDown", "ArrowUp", "ArrowLeft", "ArrowRight", "Enter", "Escape", "Tab"];
      if (block.includes(event.key)) event.preventDefault();
      if (event.key === "Escape") this.app.goto("menu");
      else if (event.key === "Tab") {
        this.mode = this.mode === "hand" ? "monster" : this.mode === "monster" ? "spell" : "hand";
        this.cursor = 0;
      } else if (event.key === "ArrowRight" || event.key === "ArrowDown") this.move(1);
      else if (event.key === "ArrowLeft" || event.key === "ArrowUp") this.move(-1);
      else if (event.key === "Enter") this.activate();
      else if (event.key.toLowerCase() === "d") this.drawCard();
      else if (event.key.toLowerCase() === "g") this.toGrave();
    }

    move(delta) {
      const max = this.mode === "hand" ? this.app.duel.hand[this.player].length : 5;
      this.cursor = max > 0 ? (this.cursor + delta + max) % max : 0;
    }

    activate() {
      if (this.mode !== "hand") return;
      this.app.status = this.app.duel.playHandAt(this.player, this.cursor)
        ? "Played card from hand"
        : "No open zone";
      this.cursor = 0;
    }

    drawCard() {
      this.app.status = this.app.duel.draw(this.player) ? "Drew a card" : "Deck empty";
    }

    toGrave() {
      if (this.mode !== "monster") return;
      this.app.status = this.app.duel.sendMonsterToGrave(this.player, this.cursor)
        ? "Sent monster to grave"
        : "No monster there";
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
      this.app.drawChrome(
        "DUEL",
        "[TAB] hand/monster/spell [ENTER] play own hand [D] draw own card [G] own monster to grave [ESC] menu",
      );
      this.layout();
      this.drawPreview(g);
      this.drawField(g);
      this.drawHands(g);
      this.drawInfo(g);
    }

    layout() {
      const leftW = Math.max(260, Math.floor(this.app.w * 0.24));
      this.preview = { x: 14, y: 72, w: leftW, h: this.app.h - 126 };
      this.field = { x: leftW + 28, y: 72, w: this.app.w - leftW - 42, h: this.app.h - 220 };
      this.oppHand = { x: this.field.x, y: this.field.y, w: this.field.w, h: 60 };
      this.ownHand = { x: this.field.x, y: this.app.h - 138, w: this.field.w, h: 84 };

      const zoneW = Math.min(82, (this.field.w - 80) / 5);
      const zoneH = zoneW * 86 / 59;
      const startX = this.field.x + (this.field.w - zoneW * 5 - 10 * 4) / 2;
      const top = this.field.y + 88;
      const bottom = this.ownHand.y - zoneH * 2 - 36;

      this.oppSpells = this.zones(startX, top, zoneW, zoneH);
      this.oppMonsters = this.zones(startX, top + zoneH + 16, zoneW, zoneH);
      this.ownMonsters = this.zones(startX, bottom, zoneW, zoneH);
      this.ownSpells = this.zones(startX, bottom + zoneH + 16, zoneW, zoneH);
    }

    zones(startX, y, w, h) {
      return Array.from({ length: 5 }, (_, i) => ({ x: startX + i * (w + 10), y, w, h }));
    }

    selectedCard() {
      if (this.mode === "hand") return this.app.duel.hand[this.player][this.cursor] || null;
      if (this.mode === "monster") return this.app.duel.monsters[this.player][this.cursor] || null;
      return this.app.duel.spells[this.player][this.cursor] || null;
    }

    drawPreview(g) {
      Ui.rect(g, this.preview, "rgba(12,16,20,.92)", "#303946");
      const card = this.selectedCard();
      g.fillStyle = "#f1f5f8";
      g.font = "700 17px system-ui";
      g.fillText("Preview", this.preview.x + 12, this.preview.y + 28);
      const w = Math.min(this.preview.w - 48, 220);
      const h = w * 86 / 59;
      const x = this.preview.x + (this.preview.w - w) / 2;
      const y = this.preview.y + 58;
      Ui.cardImage(g, this.app.images, card, x, y, w, h);
      if (!card) return;
      g.fillStyle = "#f1f5f8";
      g.font = "700 18px system-ui";
      Ui.text(g, card.name, this.preview.x + 16, y + h + 32, this.preview.w - 32);
      g.fillStyle = Ui.kindColor(card.kind);
      g.font = "14px system-ui";
      g.fillText(Card.kindTag(card.kind), this.preview.x + 16, y + h + 56);
      g.fillStyle = "#d8e0e8";
      Ui.wrap(g, card.desc, this.preview.x + 16, y + h + 84, this.preview.w - 32, 19, 10);
    }

    drawField(g) {
      Ui.rect(g, this.field, "rgba(12,16,20,.88)", "#303946");
      this.label(g, "Opponent Spell/Trap", this.oppSpells[0]);
      this.label(g, "Opponent Monsters", this.oppMonsters[0]);
      this.label(g, "Your Monsters", this.ownMonsters[0]);
      this.label(g, "Your Spell/Trap", this.ownSpells[0]);
      for (let i = 0; i < 5; i += 1) {
        this.drawZone(g, this.oppSpells[i], this.app.duel.spells[0][i], this.player === 0 && this.mode === "spell" && this.cursor === i);
        this.drawZone(g, this.oppMonsters[i], this.app.duel.monsters[0][i], this.player === 0 && this.mode === "monster" && this.cursor === i);
        this.drawZone(g, this.ownMonsters[i], this.app.duel.monsters[1][i], this.player === 1 && this.mode === "monster" && this.cursor === i);
        this.drawZone(g, this.ownSpells[i], this.app.duel.spells[1][i], this.player === 1 && this.mode === "spell" && this.cursor === i);
      }
    }

    label(g, text, zone) {
      g.fillStyle = "#9faab7";
      g.font = "13px system-ui";
      g.fillText(text, this.field.x + 16, zone.y - 8);
    }

    drawZone(g, rect, card, selected) {
      Ui.rect(g, rect, "rgba(18,24,30,.92)", selected ? "#f3d45b" : "#303946");
      if (card) Ui.cardImage(g, this.app.images, card, rect.x + 4, rect.y + 4, rect.w - 8, rect.h - 8);
    }

    drawHands(g) {
      this.drawHand(g, this.oppHand, this.app.duel.hand[0], false);
      this.drawHand(g, this.ownHand, this.app.duel.hand[1], true);
    }

    drawHand(g, rect, cards, own) {
      Ui.rect(g, rect, "rgba(12,16,20,.92)", own && this.mode === "hand" && this.player === 1 ? "#f3d45b" : "#303946");
      const w = own ? 48 : 32;
      const h = w * 86 / 59;
      for (let i = 0; i < cards.length; i += 1) {
        const x = rect.x + 12 + i * (own ? 56 : 38);
        const y = rect.y + 8;
        if (own && this.mode === "hand" && this.cursor === i) {
          g.strokeStyle = "#f3d45b";
          g.strokeRect(x - 3, y - 3, w + 6, h + 6);
        }
        own ? Ui.cardImage(g, this.app.images, cards[i], x, y, w, h)
            : Ui.cardImage(g, this.app.images, null, x, y, w, h);
      }
    }

    drawInfo(g) {
      const x = this.field.x + 16;
      const y = this.field.y + 28;
      g.fillStyle = "#f1f5f8";
      g.font = "700 17px system-ui";
      g.fillText(`Turn Player: P${this.app.duel.turnPlayer + 1}`, x, y);
      g.fillStyle = "#9faab7";
      g.font = "13px system-ui";
      g.fillText(this.playerLine(0), x, y + 22);
      g.fillText(this.playerLine(1), x, y + 40);
    }

    playerLine(player) {
      const d = this.app.duel;
      const grave = d.graveCount?.[player] ?? d.grave[player].length;
      const banished = d.banishedCount?.[player] ?? d.banished[player].length;
      return `P${player + 1} LP ${d.lp[player]}   Deck ${d.deck[player].length}   Hand ${d.hand[player].length}   GY ${grave}   Banished ${banished}`;
    }

    hitZones(zones, x, y) {
      return zones.findIndex((r) => x >= r.x && x <= r.x + r.w && y >= r.y && y <= r.y + r.h);
    }

    hitHand(rect, cards, x, y) {
      if (x < rect.x || x > rect.x + rect.w || y < rect.y || y > rect.y + rect.h) return -1;
      const index = Math.floor((x - rect.x - 12) / 56);
      return index >= 0 && index < cards.length ? index : -1;
    }
  }

  window.OpenJoeyScreens.DuelScreen = DuelScreen;
})();
