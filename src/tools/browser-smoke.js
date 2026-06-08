#!/usr/bin/env node
"use strict";

const childProcess = require("child_process");
const crypto = require("crypto");
const fs = require("fs");
const http = require("http");
const net = require("net");
const path = require("path");

const root = path.resolve(__dirname, "../..");
const sitePort = Number(process.env.OPENJOEY_TEST_PORT || 4177);
const cdpPort = Number(process.env.OPENJOEY_CDP_PORT || 9228);
const url = process.env.OPENJOEY_TEST_URL || `http://127.0.0.1:${sitePort}/`;
const outDir = process.env.OPENJOEY_TEST_OUT || path.join(root, "build", "browser-smoke");
const testScreen = process.env.OPENJOEY_TEST_SCREEN || "duel";

function parseViewport(value) {
  if (!value) return null;
  const match = String(value).match(/^(\d+)x(\d+)$/);
  if (!match) throw new Error(`Invalid OPENJOEY_TEST_VIEWPORT: ${value}`);
  return { width: Number(match[1]), height: Number(match[2]) };
}

const viewport = parseViewport(process.env.OPENJOEY_TEST_VIEWPORT);
const screenshotName = viewport
  ? `${testScreen}-${viewport.width}x${viewport.height}.png`
  : `${testScreen}.png`;
const screenshotPath = path.join(outDir, screenshotName);

function log(message) {
  console.log(`[browser-smoke] ${message}`);
}

function commandExists(command) {
  const result = childProcess.spawnSync("bash", ["-lc", `command -v ${command}`], {
    encoding: "utf8",
  });
  return result.status === 0 ? result.stdout.trim() : "";
}

function findChrome() {
  if (process.env.CHROME_BIN) return process.env.CHROME_BIN;
  return (
    commandExists("chromium-browser") ||
    commandExists("chromium") ||
    commandExists("google-chrome") ||
    commandExists("google-chrome-stable")
  );
}

function requestJson(port, requestPath) {
  return new Promise((resolve, reject) => {
    const req = http.get({ host: "127.0.0.1", port, path: requestPath }, (res) => {
      let data = "";
      res.setEncoding("utf8");
      res.on("data", (chunk) => {
        data += chunk;
      });
      res.on("end", () => {
        try {
          resolve(JSON.parse(data));
        } catch (error) {
          reject(error);
        }
      });
    });
    req.on("error", reject);
  });
}

function waitForHttp(port, requestPath, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  return new Promise((resolve, reject) => {
    const tick = () => {
      requestJson(port, requestPath)
        .then(resolve)
        .catch((error) => {
          if (Date.now() > deadline) reject(error);
          else setTimeout(tick, 120);
        });
    };
    tick();
  });
}

function waitForUrl(port, timeoutMs) {
  const deadline = Date.now() + timeoutMs;
  return new Promise((resolve, reject) => {
    const tick = () => {
      const req = http.get({ host: "127.0.0.1", port, path: "/" }, (res) => {
        res.resume();
        resolve();
      });
      req.on("error", (error) => {
        if (Date.now() > deadline) reject(error);
        else setTimeout(tick, 120);
      });
    };
    tick();
  });
}

function encodeFrame(text) {
  const payload = Buffer.from(text);
  const len = payload.length;
  let header;
  if (len < 126) header = Buffer.alloc(2);
  else if (len < 65536) header = Buffer.alloc(4);
  else header = Buffer.alloc(10);

  header[0] = 0x81;
  if (len < 126) {
    header[1] = 0x80 | len;
  } else if (len < 65536) {
    header[1] = 0x80 | 126;
    header.writeUInt16BE(len, 2);
  } else {
    header[1] = 0x80 | 127;
    header.writeBigUInt64BE(BigInt(len), 2);
  }

  const mask = crypto.randomBytes(4);
  const out = Buffer.alloc(len);
  for (let i = 0; i < len; i += 1) out[i] = payload[i] ^ mask[i % 4];
  return Buffer.concat([header, mask, out]);
}

function decodeFrames(buffer) {
  const messages = [];
  let offset = 0;

  while (offset + 2 <= buffer.length) {
    const b0 = buffer[offset];
    const b1 = buffer[offset + 1];
    let len = b1 & 0x7f;
    let header = 2;

    if (len === 126) {
      if (offset + 4 > buffer.length) break;
      len = buffer.readUInt16BE(offset + 2);
      header = 4;
    } else if (len === 127) {
      if (offset + 10 > buffer.length) break;
      len = Number(buffer.readBigUInt64BE(offset + 2));
      header = 10;
    }

    const masked = Boolean(b1 & 0x80);
    const maskLen = masked ? 4 : 0;
    if (offset + header + maskLen + len > buffer.length) break;

    let payload = buffer.subarray(offset + header + maskLen, offset + header + maskLen + len);
    if (masked) {
      const mask = buffer.subarray(offset + header, offset + header + 4);
      const tmp = Buffer.alloc(payload.length);
      for (let i = 0; i < payload.length; i += 1) tmp[i] = payload[i] ^ mask[i % 4];
      payload = tmp;
    }

    if ((b0 & 0x0f) === 1) messages.push(payload.toString("utf8"));
    offset += header + maskLen + len;
  }

  return { messages, rest: buffer.subarray(offset) };
}

class CdpClient {
  constructor(wsUrl) {
    const parsed = new URL(wsUrl);
    this.host = parsed.hostname;
    this.port = Number(parsed.port);
    this.requestPath = parsed.pathname + parsed.search;
    this.nextId = 0;
    this.pending = new Map();
    this.events = [];
    this.buffer = Buffer.alloc(0);
  }

  connect() {
    return new Promise((resolve, reject) => {
      this.socket = net.connect(this.port, this.host, () => {
        const key = crypto.randomBytes(16).toString("base64");
        this.socket.write([
          `GET ${this.requestPath} HTTP/1.1`,
          `Host: ${this.host}:${this.port}`,
          "Upgrade: websocket",
          "Connection: Upgrade",
          `Sec-WebSocket-Key: ${key}`,
          "Sec-WebSocket-Version: 13",
          "",
          "",
        ].join("\r\n"));
      });

      let handshake = Buffer.alloc(0);
      const onHandshake = (chunk) => {
        handshake = Buffer.concat([handshake, chunk]);
        const end = handshake.indexOf("\r\n\r\n");
        if (end < 0) return;

        const head = handshake.subarray(0, end).toString("utf8");
        if (!head.includes("101")) {
          reject(new Error(`DevTools WebSocket upgrade failed: ${head}`));
          return;
        }

        this.socket.off("data", onHandshake);
        this.socket.on("data", (data) => this.onData(data));
        const rest = handshake.subarray(end + 4);
        if (rest.length) this.onData(rest);
        resolve();
      };

      this.socket.on("data", onHandshake);
      this.socket.on("error", reject);
    });
  }

  onData(data) {
    this.buffer = Buffer.concat([this.buffer, data]);
    const decoded = decodeFrames(this.buffer);
    this.buffer = decoded.rest;

    for (const text of decoded.messages) {
      const message = JSON.parse(text);
      if (message.id && this.pending.has(message.id)) {
        this.pending.get(message.id)(message);
        this.pending.delete(message.id);
      } else {
        this.events.push(message);
      }
    }
  }

  send(method, params = {}) {
    const id = ++this.nextId;
    this.socket.write(encodeFrame(JSON.stringify({ id, method, params })));
    return new Promise((resolve) => {
      this.pending.set(id, resolve);
    });
  }

  close() {
    this.socket.end();
  }
}

function spawnProcess(command, args, options = {}) {
  return childProcess.spawn(command, args, {
    cwd: root,
    stdio: options.stdio || ["ignore", "pipe", "pipe"],
    env: process.env,
  });
}

function runMatrix(name) {
  const cases = name === "phone"
    ? [
        ["deck", "390x844"],
        ["deck", "844x390"],
        ["duel", "390x844"],
        ["duel", "844x390"],
      ]
    : [];
  if (!cases.length) throw new Error(`Unknown OPENJOEY_TEST_MATRIX: ${name}`);

  for (let i = 0; i < cases.length; i += 1) {
    const [screen, size] = cases[i];
    log(`matrix ${i + 1}/${cases.length}: ${screen} ${size}`);
    const env = {
      ...process.env,
      OPENJOEY_TEST_SCREEN: screen,
      OPENJOEY_TEST_VIEWPORT: size,
      OPENJOEY_TEST_PORT: String(sitePort + i + 1),
      OPENJOEY_CDP_PORT: String(cdpPort + i + 1),
    };
    delete env.OPENJOEY_TEST_MATRIX;
    const result = childProcess.spawnSync(process.execPath, [__filename], {
      cwd: root,
      env,
      stdio: "inherit",
    });
    if (result.status !== 0) process.exit(result.status || 1);
  }
  log(`${name} matrix passed`);
}

function killProcess(proc) {
  if (!proc || proc.killed) return;
  try {
    proc.kill("SIGTERM");
  } catch (_) {
    // Ignore cleanup failures.
  }
}

function internalFailureEvents(events) {
  return events.filter((event) => {
    if (event.method === "Runtime.exceptionThrown") return true;
    if (event.method === "Network.loadingFailed") {
      const urlText = event.params?.requestId || "";
      return !String(urlText).includes("images.ygoprodeck.com");
    }
    if (event.method === "Log.entryAdded") {
      const entry = event.params?.entry || {};
      if (entry.level !== "error") return false;
      const entryUrl = entry.url || "";
      return !entryUrl || entryUrl.startsWith(url);
    }
    return false;
  });
}

async function waitForApp(cdp) {
  const deadline = Date.now() + 10000;
  while (Date.now() < deadline) {
    const result = await cdp.send("Runtime.evaluate", {
      expression: "Boolean(window.openJoeyApp)",
      returnByValue: true,
    });
    if (result.result?.result?.value) return;
    await new Promise((resolve) => setTimeout(resolve, 150));
  }
  throw new Error("window.openJoeyApp was not created");
}

async function main() {
  const chrome = findChrome();
  if (!chrome) throw new Error("Chromium/Chrome not found. Set CHROME_BIN or install chromium.");

  fs.mkdirSync(outDir, { recursive: true });

  log(`serving ${root} on ${sitePort}`);
  const server = spawnProcess("python3", ["-m", "http.server", String(sitePort), "--directory", root]);

  let browser;
  try {
    await waitForUrl(sitePort, 5000);

    log(`launching ${chrome}`);
    browser = spawnProcess(chrome, [
      "--headless=new",
      `--remote-debugging-port=${cdpPort}`,
      "--disable-gpu",
      "--no-sandbox",
      "--disable-extensions",
      `--user-data-dir=${path.join("/tmp", `openjoey-browser-smoke-${process.pid}`)}`,
      url,
    ]);

    await waitForHttp(cdpPort, "/json/list", 10000);
    const pages = await requestJson(cdpPort, "/json/list");
    const page = pages.find((item) => item.type === "page" && item.url.startsWith(url));
    if (!page) throw new Error(`Could not find test page for ${url}`);

    const cdp = new CdpClient(page.webSocketDebuggerUrl);
    await cdp.connect();
    await cdp.send("Runtime.enable");
    await cdp.send("Page.enable");
    await cdp.send("Log.enable");
    await cdp.send("Network.enable");
    if (viewport) {
      await cdp.send("Emulation.setDeviceMetricsOverride", {
        width: viewport.width,
        height: viewport.height,
        deviceScaleFactor: 1,
        mobile: viewport.width < 700,
      });
    }

    await waitForApp(cdp);
    await cdp.send("Runtime.evaluate", { expression: `window.openJoeyApp.goto(${JSON.stringify(testScreen)})` });
    await new Promise((resolve) => setTimeout(resolve, 1800));

    const expression = `JSON.stringify((() => {
      const app = window.openJoeyApp;
      return {
        title: document.title,
        appReady: Boolean(app),
        backend: app && app.backend,
        status: app && app.status,
        screen: app && app.screen && app.screen.constructor.name,
        cardCount: app && app.cardDb && app.cardDb.cards.length,
        field: app && app.screen && app.screen.field,
        ownHand: app && app.screen && app.screen.ownHand,
        preview: app && app.screen && app.screen.preview,
        pool: app && app.screen && app.screen.pool,
        deck: app && app.screen && app.screen.deck,
        layoutMode: app && app.screen && app.screen.layoutMode
      };
    })())`;

    const stateResult = await cdp.send("Runtime.evaluate", {
      expression,
      returnByValue: true,
    });
    const state = JSON.parse(stateResult.result.result.value);

    const screenshot = await cdp.send("Page.captureScreenshot", {
      format: "png",
      captureBeyondViewport: false,
    });
    fs.writeFileSync(screenshotPath, Buffer.from(screenshot.result.data, "base64"));

    const events = cdp.events.filter((event) => (
      event.method === "Runtime.exceptionThrown" ||
      event.method === "Log.entryAdded" ||
      event.method === "Network.loadingFailed"
    ));
    const failures = internalFailureEvents(events);

    cdp.close();

    if (!state.appReady) throw new Error("App did not initialize");
    if (state.backend !== "C++ WASM") throw new Error(`Expected C++ WASM backend, got ${state.backend}`);
    const expectedScreen = testScreen === "deck" ? "DeckEditorScreen" : testScreen === "duel" ? "DuelScreen" : null;
    if (expectedScreen && state.screen !== expectedScreen) throw new Error(`Expected ${expectedScreen}, got ${state.screen}`);
    if (!state.status || !state.status.includes("C++ WASM")) throw new Error(`Unexpected status: ${state.status}`);
    if (!state.cardCount || state.cardCount < 1000) throw new Error(`Unexpected card count: ${state.cardCount}`);
    if (testScreen === "duel" && (!state.field || state.field.h < 100)) {
      throw new Error(`Duel field is too small: ${JSON.stringify(state.field)}`);
    }
    if (testScreen === "deck") {
      const rects = [state.pool, state.deck].filter(Boolean);
      for (const rect of rects) {
        if (rect.w < 120 || rect.h < 120) throw new Error(`Deck editor panel is too small: ${JSON.stringify(state)}`);
      }
    }
    if (failures.length) throw new Error(`Browser errors: ${JSON.stringify(failures, null, 2)}`);

    log(`backend: ${state.backend}`);
    log(`screen: ${state.screen}`);
    if (state.layoutMode) log(`layout: ${state.layoutMode}`);
    log(`cards: ${state.cardCount}`);
    log(`screenshot: ${screenshotPath}`);
    log("passed");
  } finally {
    killProcess(browser);
    killProcess(server);
  }
}

const run = process.env.OPENJOEY_TEST_MATRIX
  ? Promise.resolve().then(() => runMatrix(process.env.OPENJOEY_TEST_MATRIX))
  : main();

run.catch((error) => {
  console.error(`[browser-smoke] failed: ${error.stack || error.message || error}`);
  process.exit(1);
});
