const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..", "..");
const input = path.join(root, "data", "cards.json");
const output = path.join(root, "src", "web", "src", "domain", "generated", "cardRows.generated.js");
const defaultUrl = "https://db.ygoprodeck.com/api/v7/cardinfo.php";
const bundle = process.argv.includes("--bundle") || process.env.OPENJOEY_BUNDLE_CARD_DB === "1";

if (!bundle) {
  fs.writeFileSync(
    output,
    [
      `window.OPENJOEY_DEFAULT_CARD_DB_URL = ${JSON.stringify(defaultUrl)};`,
      "window.OPENJOEY_CARD_ROWS = [];",
      "",
    ].join("\n"),
  );
  console.log(`wrote remote card DB bootstrap to ${output}`);
  process.exit(0);
}

const raw = JSON.parse(fs.readFileSync(input, "utf8"));
const cards = raw.data
  .filter((card) => card && card.id && card.name)
  .map((card) => {
    const kind = card.frameType === "spell" ? 1 : card.frameType === "trap" ? 2 : 0;
    return [
      card.id,
      card.card_images?.[0]?.id || card.id,
      kind,
      card.name,
      card.desc || "",
      card.atk || 0,
      card.def || 0,
      card.level || card.rank || 0,
      card.humanReadableCardType || card.type || "",
    ];
  });

fs.writeFileSync(
  output,
  [
    `window.OPENJOEY_DEFAULT_CARD_DB_URL = ${JSON.stringify(defaultUrl)};`,
    `window.OPENJOEY_CARD_ROWS = ${JSON.stringify(cards, null, 2)};`,
    "",
  ].join("\n"),
);
console.log(`wrote ${cards.length} bundled cards to ${output}`);
