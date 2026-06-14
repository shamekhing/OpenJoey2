(function () {
  const DB_NAME = "openjoey2-card-db";
  const STORE_NAME = "kv";
  const ROWS_KEY = "rows";

  function openDb() {
    if (!("indexedDB" in window)) return Promise.resolve(null);
    return new Promise((resolve) => {
      const request = indexedDB.open(DB_NAME, 1);
      request.onupgradeneeded = () => {
        request.result.createObjectStore(STORE_NAME);
      };
      request.onsuccess = () => resolve(request.result);
      request.onerror = () => resolve(null);
    });
  }

  async function getValue(key) {
    const db = await openDb();
    if (!db) return null;
    return new Promise((resolve) => {
      const request = db.transaction(STORE_NAME, "readonly").objectStore(STORE_NAME).get(key);
      request.onsuccess = () => resolve(request.result || null);
      request.onerror = () => resolve(null);
    });
  }

  async function setValue(key, value) {
    const db = await openDb();
    if (!db) return false;
    return new Promise((resolve) => {
      const request = db.transaction(STORE_NAME, "readwrite").objectStore(STORE_NAME).put(value, key);
      request.onsuccess = () => resolve(true);
      request.onerror = () => resolve(false);
    });
  }

  function loadRows() {
    return getValue(ROWS_KEY);
  }

  function saveRows(rows) {
    return setValue(ROWS_KEY, rows);
  }

  async function clearRows() {
    const db = await openDb();
    if (!db) return false;
    return new Promise((resolve) => {
      const request = db.transaction(STORE_NAME, "readwrite").objectStore(STORE_NAME).delete(ROWS_KEY);
      request.onsuccess = () => resolve(true);
      request.onerror = () => resolve(false);
    });
  }

  window.OpenJoeyCardDbCache = { loadRows, saveRows, clearRows };
})();
