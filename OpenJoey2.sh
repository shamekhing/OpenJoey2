#!/usr/bin/env bash
#
# OpenJoey2 build/run helper
#
# Flags:
#   -s   Setup — configure both debug and release CMake trees
#   -b   Build debug (does NOT run; for use with VS Code debugger F5)
#   -B   Build release
#   -x   Execute debug in terminal (auto-builds if binary missing)
#   -X   Execute release in terminal (auto-builds if binary missing)
#   -c   Clean both build trees
#   -h   Show this help
#
# Flags are processed in order; combine freely:
#   ./oj2.sh -s -b         setup & build debug, ready for VS Code F5
#   ./oj2.sh -b -x         build debug then execute in terminal
#   ./oj2.sh -c -s -B      clean, reconfigure, build release
#
# The raylib/FetchContent cache is shared with the sibling build directory
# (../build/_deps) so dependencies are not re-downloaded on reconfigure.

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}"

DEBUG_DIR="${ROOT_DIR}/build/debug"
RELEASE_DIR="${ROOT_DIR}/build/release"
DEPS_DIR="${ROOT_DIR}/../build/_deps"

BIN_DEBUG="${DEBUG_DIR}/OpenJoey2"
BIN_RELEASE="${RELEASE_DIR}/OpenJoey2"

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
  echo "[clean] removing build/debug and build/release..."
  rm -rf "${DEBUG_DIR}" "${RELEASE_DIR}"
  echo "[clean] done."
}

[[ $# -eq 0 ]] && { help_msg; exit 0; }

while getopts ":sbBxXch" opt; do
  case "${opt}" in
    s) setup_debug; setup_release ;;
    b) build_debug ;;
    B) build_release ;;
    x) exec_debug ;;
    X) exec_release ;;
    c) do_clean ;;
    h) help_msg ;;
    :) echo "Option -${OPTARG} requires an argument."; exit 1 ;;
    \?) echo "Unknown flag: -${OPTARG}"; help_msg; exit 1 ;;
  esac
done
