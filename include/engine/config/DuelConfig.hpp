#pragma once
// ── DuelConfig — the classic ruleset as CONFIGURATION (rules = match config) ─
// Every numeric rule of the format lives here exactly once (the engine, the
// UI and the future openjoey-ai read these). A different game later = a
// different preset, zero engine changes.
#include <cstdint>
#include <random>

namespace openjoey::engine {

struct DuelConfig {
  // ── Match setup ────────────────────────────────────────────────────────────
  static constexpr int START_LP        = 8000;
  static constexpr int START_HAND      = 5;
  static constexpr int HAND_LIMIT      = 6;  // End Phase ("until you have 6")
  static constexpr int DECK_MIN        = 40;
  static constexpr int DECK_MAX        = 60;
  static constexpr int EXTRA_DECK_MAX  = 15;
  static constexpr int SIDE_DECK_MAX   = 15;
  static constexpr int CARDS_PER_DRAW  = 1;

  // ── First-turn restrictions (starting player) ──────────────────────────────
  static constexpr int TURNS_SKIP_DRAW   = 1;
  static constexpr int TURNS_SKIP_BATTLE = 1;

  // ── Format switches (RUNTIME fields — per-duel, settings-exposed) ─────────
  // Defaults reproduce the shipped classic behavior.
  bool chainResponseWindow = false; // p.45: chains resolve only after both pass
  bool autoDiscardEndPhase = true;  // p.41: EndTurn auto-discards to 6

  // ── Battle ─────────────────────────────────────────────────────────────────
  static constexpr int ATTACKS_PER_MONSTER = 1;
  static constexpr int TRIBUTE_LV1_4    = 0;
  static constexpr int TRIBUTE_LV5_6    = 1;
  static constexpr int TRIBUTE_LV7_PLUS = 2;
  static int tributesFor(int level) {
    if (level >= 7) return TRIBUTE_LV7_PLUS;
    if (level >= 5) return TRIBUTE_LV5_6;
    return TRIBUTE_LV1_4;
  }

  // ── Seeded randomness (deterministic episodes for the future RL agent) ─────
  std::mt19937 rng{std::random_device{}()};
  DuelConfig() = default;
  explicit DuelConfig(uint32_t seed) : rng(seed) {}
  int flipCoin() { return int(rng() % 2); }        // 0 heads / 1 tails
  int rollDie() { return int(rng() % 6) + 1; }     // d6
  int determineFirstPlayer() { return flipCoin(); }
};

} // namespace openjoey::engine
