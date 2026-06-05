(async function () {
  const rows = window.OPENJOEY_CARD_ROWS || [];
  const cardDb = new window.OpenJoeyCardDb.CardDb(rows);
  const { core, duel, backend } = await window.OpenJoeyDeckBridge.createDeckCore(cardDb);
  new window.OpenJoeyApp.App(
    document.getElementById("app"),
    document.getElementById("search"),
    cardDb,
    core,
    duel,
    backend,
  );
})();
