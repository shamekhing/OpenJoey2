/**
 * Browser bootstrap.
 *
 * Generated card rows are parsed in JavaScript, then DeckBridge chooses the
 * WASM core when available and falls back to the JS shim when it is not.
 */
(async function () {
  const rows = window.OPENJOEY_CARD_ROWS || [];
  const cardDb = new window.OpenJoeyCardDb.CardDb(rows);
  const { core, duel, backend } = await window.OpenJoeyDeckBridge.createDeckCore(cardDb);
  window.openJoeyApp = new window.OpenJoeyApp.App(
    document.getElementById("app"),
    document.getElementById("search"),
    cardDb,
    core,
    duel,
    backend,
  );
})();
