#!/usr/bin/env bash

set -e

PROJECT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$PROJECT_DIR/build"
DEBUG_DIR="$BUILD_DIR/debug"
RELEASE_DIR="$BUILD_DIR/release"

case "$1" in

    build)
        cmake -S "$PROJECT_DIR" -B "$DEBUG_DIR" \
            -DCMAKE_BUILD_TYPE=Debug

        cmake --build "$DEBUG_DIR" -j"$(nproc)"
        ;;

    debug)
        cmake -S "$PROJECT_DIR" -B "$DEBUG_DIR" \
            -DCMAKE_BUILD_TYPE=Debug

        cmake --build "$DEBUG_DIR" -j"$(nproc)"

        gdb "$DEBUG_DIR/OpenJoey2"
        ;;

    release)
        cmake -S "$PROJECT_DIR" -B "$RELEASE_DIR" \
            -DCMAKE_BUILD_TYPE=Release

        cmake --build "$RELEASE_DIR" -j"$(nproc)"
        ;;

    run)
        "$DEBUG_DIR/OpenJoey2"
        ;;

    run-release)
        "$RELEASE_DIR/OpenJoey2"
        ;;

    clean)
        rm -rf "$BUILD_DIR"
        ;;

    rebuild)
        rm -rf "$BUILD_DIR"

        cmake -S "$PROJECT_DIR" -B "$DEBUG_DIR" \
            -DCMAKE_BUILD_TYPE=Debug

        cmake --build "$DEBUG_DIR" -j"$(nproc)"
        ;;

    *)
        echo "OpenJoey build frontend"
        echo
        echo "Usage: ./oj <command>"
        echo
        echo "Commands:"
        echo "  build          Build Debug"
        echo "  debug          Build Debug and launch GDB"
        echo "  release        Build Release"
        echo "  run            Run Debug build"
        echo "  run-release    Run Release build"
        echo "  rebuild        Clean and build Debug"
        echo "  clean          Remove build directory"
        ;;

esac