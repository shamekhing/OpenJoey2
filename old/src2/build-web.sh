#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")" && pwd)"
OUT="$ROOT/web/openjoey_core.js"

emcc \
  "$ROOT/cpp/DeckCore.cpp" \
  "$ROOT/cpp/WasmApi.cpp" \
  -I"$ROOT/cpp" \
  -I"$ROOT/cpp/openjoey" \
  -I"$ROOT/cpp/openjoey/third_party" \
  -std=c++17 \
  -O3 \
  -s MODULARIZE=1 \
  -s EXPORT_NAME=createOpenJoeyCore \
  -s SINGLE_FILE=1 \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
  -s EXPORTED_FUNCTIONS='["_oj_deck_new","_oj_deck_free","_oj_deck_add","_oj_deck_remove_at","_oj_deck_clear","_oj_deck_count","_oj_deck_count_copies","_oj_deck_can_duel","_oj_deck_card_id","_oj_deck_stats_total","_oj_deck_stats_monsters","_oj_deck_stats_spells","_oj_deck_stats_traps","_oj_game_new","_oj_game_free","_oj_game_clear_decks","_oj_game_add_deck_card","_oj_game_start","_oj_game_turn_player","_oj_game_phase","_oj_game_life_points","_oj_game_winner","_oj_game_deck_count","_oj_game_hand_count","_oj_game_grave_count","_oj_game_banished_count","_oj_game_hand_card_id","_oj_game_monster_zone_id","_oj_game_spell_zone_id","_oj_game_extra_monster_zone_id","_oj_game_draw","_oj_game_play_hand_at","_oj_game_send_monster_to_grave","_oj_game_advance_phase"]' \
  -o "$OUT"

echo "wrote $OUT"
