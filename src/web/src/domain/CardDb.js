(function () {
  const KIND_MONSTER = 0;
  const KIND_SPELL = 1;
  const KIND_TRAP = 2;

  function kindTag(kind) {
    if (kind === KIND_SPELL) return "[SPL]";
    if (kind === KIND_TRAP) return "[TRP]";
    return "[MON]";
  }

  function kindName(kind) {
    if (kind === KIND_SPELL) return "Spell";
    if (kind === KIND_TRAP) return "Trap";
    return "Monster";
  }

  function statsLine(card, isShort = false) {
    if (!card || card.kind !== KIND_MONSTER) return "";
    return isShort
      ? `L${card.level} ${card.atk}/${card.def}`
      : `Level ${card.level}  ATK ${card.atk}  DEF ${card.def}`;
  }

  function imageUrl(card) {
    return `https://images.ygoprodeck.com/images/cards/${card.imageId}.jpg`;
  }

  class CardDb {
    constructor(rows) {
      this.cards = rows.map((row, index) => ({
        index,
        id: row[0],
        imageId: row[1],
        kind: row[2],
        name: row[3],
        desc: row[4],
        atk: row[5],
        def: row[6],
        level: row[7],
        readableType: row[8],
      }));
      this.byId = new Map(this.cards.map((card) => [card.id, card]));
      this.sorted = [...this.cards];
    }

    rebuildSort(mode) {
      const byName = (a, b) => a.name.localeCompare(b.name);
      const sorters = [
        (a, b) => a.kind - b.kind || byName(a, b),
        byName,
        (a, b) => b.name.localeCompare(a.name),
        (a, b) => b.level - a.level || byName(a, b),
        (a, b) => a.level - b.level || byName(a, b),
        (a, b) => b.atk - a.atk || byName(a, b),
        (a, b) => a.atk - b.atk || byName(a, b),
        (a, b) => b.def - a.def || byName(a, b),
        (a, b) => a.def - b.def || byName(a, b),
        (a, b) => a.id - b.id,
      ];
      this.sorted = [...this.cards].sort(sorters[mode] || sorters[0]);
    }

    filter(query, typeFilter) {
      const q = query.trim().toLowerCase();
      return this.sorted.filter((card) => {
        if (typeFilter > 0 && card.kind !== typeFilter - 1) return false;
        return !q || card.name.toLowerCase().includes(q);
      });
    }
  }

  window.OpenJoeyCardDb = {
    CardDb,
    KIND_MONSTER,
    KIND_SPELL,
    KIND_TRAP,
    kindTag,
    kindName,
    statsLine,
    imageUrl,
  };
})();
