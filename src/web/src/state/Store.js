(function () {
  class Store {
    constructor(initialState = {}) {
      this.state = initialState;
      this.listeners = [];
    }

    get() {
      return this.state;
    }

    set(patch) {
      this.state = { ...this.state, ...patch };
      for (const listener of this.listeners) listener(this.state);
    }

    subscribe(listener) {
      this.listeners.push(listener);
      return () => {
        this.listeners = this.listeners.filter((entry) => entry !== listener);
      };
    }
  }

  window.OpenJoeyStore = { Store };
})();
