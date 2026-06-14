#!/usr/bin/env bash
#
# OpenJoey2 root build/run helper
#
# Flags:
#   -s   Setup native CMake debug/release trees from the repo root
#   -b   Build native debug
#   -B   Build native release
#   -x   Execute native debug
#   -X   Execute native release
#   -w   Build web/WASM module into src/web/src/domain/generated/
#   -r   Run web app with BusyBox httpd at http://localhost:8080
#   -W   Build web/WASM, then run web app
#   -n   Build/run root CMake release smoke target when one exists
#   -t   Run automated browser smoke test
#   -m   Measure src/web deploy size and gzip estimates
#   -d   Generate small remote/cached card DB bootstrap
#   -D   Generate large bundled card DB rows for offline/dev
#   -c   Clean root build trees
#   -h   Show this help
#
# Flags are processed in order; combine freely:
#   ./OpenJoey2.sh -w       build web/WASM
#   ./OpenJoey2.sh -r       run web app
#   ./OpenJoey2.sh -W       build web/WASM and run it
#   ./OpenJoey2.sh -s -b    setup & build native debug
#   ./OpenJoey2.sh -c -s -B clean, reconfigure, build native release
#
# Web server port:
#   OPENJOEY_PORT=9090 ./OpenJoey2.sh -r
#
# Build and run entry points live here at the repo root. Do not add .sh or
# CMake entry points under src/.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"

DEBUG_DIR="${ROOT_DIR}/build/debug"
RELEASE_DIR="${ROOT_DIR}/build/release"
WEB_BUILD_DIR="${ROOT_DIR}/build/web-native"
DEPS_DIR="${ROOT_DIR}/../build/_deps"
SRC_DIR="${ROOT_DIR}/src"
WEB_OUT="${SRC_DIR}/web/src/domain/generated/openJoeyCore.generated.js"
WEB_DIR="${SRC_DIR}/web"
WEB_PORT="${OPENJOEY_PORT:-8080}"
CARD_DB_GENERATOR="${SRC_DIR}/tools/generate-card-db.js"

BIN_DEBUG="${DEBUG_DIR}/OpenJoey2"
BIN_RELEASE="${RELEASE_DIR}/OpenJoey2"
SMOKE_BIN="${WEB_BUILD_DIR}/OpenJoey2"

JOBS="$(nproc 2>/dev/null || echo 4)"

help_msg() {
  grep '^#' "$0" | grep -v '^#!/' | sed 's/^# \{0,2\}//'
}

setup_debug() {
  echo "[setup] configuring debug..."
  cmake -S "${ROOT_DIR}" -B "${DEBUG_DIR}" \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_CXX_FLAGS_DEBUG="-O0 -g3 -ggdb -fno-omit-frame-pointer -fno-inline -fno-optimize-sibling-calls" \
    -DFETCHCONTENT_BASE_DIR="${DEPS_DIR}"
}

setup_release() {
  echo "[setup] configuring release..."
  cmake -S "${ROOT_DIR}" -B "${RELEASE_DIR}" \
    -DCMAKE_BUILD_TYPE=Release \
    -DFETCHCONTENT_BASE_DIR="${DEPS_DIR}"
}

build_debug() {
  [[ ! -f "${DEBUG_DIR}/CMakeCache.txt" ]] && setup_debug
  echo "[build] debug..."
  cmake --build "${DEBUG_DIR}" -j"${JOBS}"
}

build_release() {
  [[ ! -f "${RELEASE_DIR}/CMakeCache.txt" ]] && setup_release
  echo "[build] release..."
  cmake --build "${RELEASE_DIR}" -j"${JOBS}"
}

build_web() {
  if ! command -v emcc >/dev/null 2>&1; then
    echo "[web] emcc not found. Install/activate Emscripten first." >&2
    exit 1
  fi
  echo "[web] building ${WEB_OUT}..."
  emcc \
    "${SRC_DIR}/cpp/WasmApi.cpp" \
    -I"${SRC_DIR}/cpp" \
    -std=c++17 \
    -O0 \
    -s MODULARIZE=1 \
    -s EXPORT_NAME=createOpenJoeyCore \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s EXPORTED_RUNTIME_METHODS='["cwrap"]' \
    -s EXPORTED_FUNCTIONS='["_oj_deck_new","_oj_deck_free","_oj_deck_add","_oj_deck_remove_at","_oj_deck_clear","_oj_deck_count","_oj_deck_count_copies","_oj_deck_can_duel","_oj_deck_card_id","_oj_deck_stats_total","_oj_deck_stats_monsters","_oj_deck_stats_spells","_oj_deck_stats_traps","_oj_game_new","_oj_game_free","_oj_game_clear_decks","_oj_game_add_deck_card","_oj_game_start","_oj_game_turn_player","_oj_game_phase","_oj_game_life_points","_oj_game_winner","_oj_game_deck_count","_oj_game_hand_count","_oj_game_grave_count","_oj_game_banished_count","_oj_game_hand_card_id","_oj_game_monster_zone_id","_oj_game_spell_zone_id","_oj_game_extra_monster_zone_id","_oj_game_draw","_oj_game_play_hand_at","_oj_game_send_monster_to_grave","_oj_game_advance_phase"]' \
    -o "${WEB_OUT}"
  echo "[web] wrote ${WEB_OUT}"
}

run_web() {
  if ! command -v busybox >/dev/null 2>&1; then
    echo "[web] busybox not found. Install BusyBox or use another static server." >&2
    exit 1
  fi
  if [[ ! -f "${WEB_DIR}/index.html" ]]; then
    echo "[web] missing ${WEB_DIR}/index.html" >&2
    exit 1
  fi
  echo "[web] serving ${WEB_DIR}"
  echo "[web] open http://localhost:${WEB_PORT}"
  echo "[web] press Ctrl+C to stop"
  cd "${WEB_DIR}"
  exec busybox httpd -f -p "${WEB_PORT}" -h .
}

build_and_run_web() {
  build_web
  run_web
}

native_smoke() {
  echo "[smoke] configuring root CMake release..."
  cmake -S "${ROOT_DIR}" -B "${WEB_BUILD_DIR}" -DCMAKE_BUILD_TYPE=Release
  echo "[smoke] building..."
  cmake --build "${WEB_BUILD_DIR}" -j"${JOBS}"
  if [[ -x "${SMOKE_BIN}" ]]; then
    echo "[smoke] ${SMOKE_BIN}"
    (cd "${ROOT_DIR}" && exec "${SMOKE_BIN}")
    return
  fi
  echo "[smoke] no OpenJoey2 binary produced by root CMake."
}

browser_smoke() {
  echo "[browser] automated smoke test..."
  node "${SRC_DIR}/tools/browser-smoke.js"
}

generate_card_db_bootstrap() {
  echo "[web] generating small remote/cached card DB bootstrap..."
  node "${CARD_DB_GENERATOR}"
}

generate_card_db_bundle() {
  echo "[web] generating large bundled card DB rows..."
  node "${CARD_DB_GENERATOR}" --bundle
}

format_bytes() {
  if command -v numfmt >/dev/null 2>&1; then
    numfmt --to=iec --suffix=B "$1"
  else
    printf "%sB" "$1"
  fi
}

measure_web_size() {
  echo "[size] deploy folder"
  du -sh "${WEB_DIR}"
  echo
  echo "[size] largest files"
  find "${WEB_DIR}" -type f -printf '%s\t%p\n' |
    sort -nr |
    head -20 |
    while IFS=$'\t' read -r bytes path; do
      printf "%10s  %s\n" "$(format_bytes "${bytes}")" "${path#${ROOT_DIR}/}"
    done
  echo
  echo "[size] gzip estimates for shipped text/wasm assets"
  find "${WEB_DIR}" -type f \( \
      -name '*.html' -o \
      -name '*.css' -o \
      -name '*.js' -o \
      -name '*.svg' -o \
      -name '*.wasm' \
    \) -print |
    while read -r path; do
      raw="$(wc -c < "${path}")"
      gzip_bytes="$(gzip -c "${path}" | wc -c)"
      printf "%10s raw  %10s gzip  %s\n" \
        "$(format_bytes "${raw}")" \
        "$(format_bytes "${gzip_bytes}")" \
        "${path#${ROOT_DIR}/}"
    done |
    sort -hr
}

exec_debug() {
  [[ ! -x "${BIN_DEBUG}" ]] && build_debug
  echo "[exec] ${BIN_DEBUG}"
  (cd "${ROOT_DIR}" && exec "${BIN_DEBUG}")
}

exec_release() {
  [[ ! -x "${BIN_RELEASE}" ]] && build_release
  echo "[exec] ${BIN_RELEASE}"
  (cd "${ROOT_DIR}" && exec "${BIN_RELEASE}")
}

do_clean() {
  echo "[clean] removing root build trees..."
  rm -rf "${DEBUG_DIR}" "${RELEASE_DIR}" "${WEB_BUILD_DIR}"
  echo "[clean] done."
}

[[ $# -eq 0 ]] && { help_msg; exit 0; }

while getopts ":sbBxXwWrntmdDch" opt; do
  case "${opt}" in
    s) setup_debug; setup_release ;;
    b) build_debug ;;
    B) build_release ;;
    x) exec_debug ;;
    X) exec_release ;;
    w) build_web ;;
    W) build_and_run_web ;;
    r) run_web ;;
    n) native_smoke ;;
    t) browser_smoke ;;
    m) measure_web_size ;;
    d) generate_card_db_bootstrap ;;
    D) generate_card_db_bundle ;;
    c) do_clean ;;
    h) help_msg ;;
    :) echo "Option -${OPTARG} requires an argument."; exit 1 ;;
    \?) echo "Unknown flag: -${OPTARG}"; help_msg; exit 1 ;;
  esac
done
