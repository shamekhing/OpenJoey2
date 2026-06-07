const fs = require("fs");
const path = require("path");

const root = path.resolve(__dirname, "..", "..");
const input = path.join(root, "data", "cards.json");
const output = path.join(root, "src2", "web", "cards-db.js");

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
  `window.OPENJOEY_CARD_ROWS=${JSON.stringify(cards)};\n`,
);
console.log(`wrote ${cards.length} cards to ${output}`);
