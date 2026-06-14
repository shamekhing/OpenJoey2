(function () {
  class JsDeckCore {
    constructor() {
      this.cards = [];
    }

    add(card) {
      if (this.cards.length >= 60) return false;
      if (this.countCopies(card.id) >= 3) return false;
      this.cards.push(card);
      return true;
    }

    removeAt(index) {
      if (index < 0 || index >= this.cards.length) return false;
      this.cards.splice(index, 1);
      return true;
    }

    clear() {
      this.cards = [];
    }

    countCopies(id) {
      return this.cards.reduce((n, card) => n + (card.id === id ? 1 : 0), 0);
    }

    canDuel() {
      return this.cards.length >= 40 && this.cards.length <= 60;
    }

    stats() {
      const out = { total: this.cards.length, monsters: 0, spells: 0, traps: 0 };
      for (const card of this.cards) {
        if (card.kind === 1) out.spells += 1;
        else if (card.kind === 2) out.traps += 1;
        else out.monsters += 1;
      }
      return out;
    }
  }

  class JsDuelCore {
    constructor() {
      this.clear();
    }

    clear() {
      this.deck = [[], []];
      this.hand = [[], []];
      this.grave = [[], []];
      this.banished = [[], []];
      this.monsters = [emptyZones(), emptyZones()];
      this.spells = [emptyZones(), emptyZones()];
      this.lp = [8000, 8000];
      this.turnPlayer = 0;
      this.phase = 0;
    }

    loadDeck(p0, p1) {
      this.clear();
      this.deck[0] = [...p0].slice(0, 60);
      this.deck[1] = [...p1].slice(0, 60);
    }

    start() {
      this.hand = [[], []];
      this.grave = [[], []];
      this.banished = [[], []];
      this.monsters = [emptyZones(), emptyZones()];
      this.spells = [emptyZones(), emptyZones()];
      for (let p = 0; p < 2; p += 1) {
        for (let i = 0; i < 5; i += 1) this.draw(p);
      }
      return this.hand[0].length > 0 || this.hand[1].length > 0;
    }

    draw(player = 0) {
      const card = this.deck[player].pop();
      if (!card) return false;
      this.hand[player].push(card);
      return true;
    }

    playHandAt(player, index) {
      const card = this.hand[player][index];
      if (!card) return false;
      const zones = card.kind === 0 ? this.monsters[player] : this.spells[player];
      const zone = zones.findIndex((entry) => !entry);
      if (zone < 0) return false;
      zones[zone] = card;
      this.hand[player].splice(index, 1);
      return true;
    }

    sendMonsterToGrave(player, zone) {
      const card = this.monsters[player][zone];
      if (!card) return false;
      this.grave[player].push(card);
      this.monsters[player][zone] = null;
      return true;
    }

    advancePhase() {
      const wasEnd = this.phase === 5;
      this.phase = this.phase >= 5 ? 0 : this.phase + 1;
      if (wasEnd) this.turnPlayer = 1 - this.turnPlayer;
      if (this.phase === 0) this.draw(this.turnPlayer);
      return this.phase;
    }
  }

  class WasmDeckCore {
    constructor(module, cardDb) {
      this.module = module;
      this.cardDb = cardDb;
      this.handle = module._oj_deck_new();
      this.cards = [];
    }

    dispose() {
      if (this.handle) this.module._oj_deck_free(this.handle);
      this.handle = 0;
    }

    add(card) {
      const ok = !!this.module._oj_deck_add(
        this.handle,
        card.id,
        card.imageId,
        card.kind,
        card.atk,
        card.def,
        card.level,
      );
      if (ok) this.cards.push(card);
      return ok;
    }

    removeAt(index) {
      const ok = !!this.module._oj_deck_remove_at(this.handle, index);
      if (ok) this.cards.splice(index, 1);
      return ok;
    }

    clear() {
      this.module._oj_deck_clear(this.handle);
      this.cards = [];
    }

    countCopies(id) {
      return this.module._oj_deck_count_copies(this.handle, id);
    }

    canDuel() {
      return !!this.module._oj_deck_can_duel(this.handle);
    }

    stats() {
      return {
        total: this.module._oj_deck_stats_total(this.handle),
        monsters: this.module._oj_deck_stats_monsters(this.handle),
        spells: this.module._oj_deck_stats_spells(this.handle),
        traps: this.module._oj_deck_stats_traps(this.handle),
      };
    }
  }

  class WasmDuelCore {
    constructor(module, cardDb) {
      this.module = module;
      this.cardDb = cardDb;
      this.handle = module._oj_game_new();
      this.deck = [[], []];
      this.hand = [[], []];
      this.graveCount = [0, 0];
      this.banishedCount = [0, 0];
      this.monsters = [emptyZones(), emptyZones()];
      this.spells = [emptyZones(), emptyZones()];
      this.lp = [8000, 8000];
      this.turnPlayer = 0;
      this.phase = 0;
    }

    dispose() {
      if (this.handle) this.module._oj_game_free(this.handle);
      this.handle = 0;
    }

    loadDeck(p0, p1) {
      this.module._oj_game_clear_decks(this.handle);
      const decks = [[...p0].slice(0, 60), [...p1].slice(0, 60)];
      for (let player = 0; player < 2; player += 1) {
        for (const card of decks[player]) {
          this.module._oj_game_add_deck_card(
            this.handle,
            player,
            card.id,
            card.imageId,
            card.kind,
            card.atk,
            card.def,
            card.level,
          );
        }
      }
      this.deck = decks;
      this.sync();
    }

    start() {
      const ok = !!this.module._oj_game_start(this.handle);
      this.sync();
      return ok;
    }

    draw(player = 0) {
      const ok = !!this.module._oj_game_draw(this.handle, player);
      this.sync();
      return ok;
    }

    playHandAt(player, index) {
      const ok = !!this.module._oj_game_play_hand_at(this.handle, player, index);
      this.sync();
      return ok;
    }

    sendMonsterToGrave(player, zone) {
      const ok = !!this.module._oj_game_send_monster_to_grave(this.handle, player, zone);
      this.sync();
      return ok;
    }

    advancePhase() {
      const phase = this.module._oj_game_advance_phase(this.handle);
      this.sync();
      return phase;
    }

    sync() {
      this.turnPlayer = this.module._oj_game_turn_player(this.handle);
      this.phase = this.module._oj_game_phase(this.handle);
      for (let player = 0; player < 2; player += 1) {
        this.lp[player] = this.module._oj_game_life_points(this.handle, player);
        this.deck[player] = new Array(this.module._oj_game_deck_count(this.handle, player)).fill(null);
        this.graveCount[player] = this.module._oj_game_grave_count(this.handle, player);
        this.banishedCount[player] = this.module._oj_game_banished_count(this.handle, player);
        this.hand[player] = [];
        const handCount = this.module._oj_game_hand_count(this.handle, player);
        for (let i = 0; i < handCount; i += 1) {
          this.hand[player].push(this.cardDb.byId.get(this.module._oj_game_hand_card_id(this.handle, player, i)) || null);
        }
        for (let i = 0; i < 5; i += 1) {
          this.monsters[player][i] = this.cardDb.byId.get(this.module._oj_game_monster_zone_id(this.handle, player, i)) || null;
          this.spells[player][i] = this.cardDb.byId.get(this.module._oj_game_spell_zone_id(this.handle, player, i)) || null;
        }
      }
    }
  }

  function emptyZones() {
    return [null, null, null, null, null];
  }

  async function createDeckCore(cardDb) {
    if (typeof createOpenJoeyCore === "function") {
      try {
        const module = await createOpenJoeyCore();
        return {
          core: new WasmDeckCore(module, cardDb),
          duel: new WasmDuelCore(module, cardDb),
          backend: "C++ WASM",
        };
      } catch (error) {
        console.warn("WASM core unavailable, using JS deck core", error);
      }
    }
    return { core: new JsDeckCore(), duel: new JsDuelCore(), backend: "JS fallback" };
  }

  window.OpenJoeyDeckBridge = { createDeckCore };
})();
