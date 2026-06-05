#!/usr/bin/env bash
set -euo pipefail

cmake -S "$(dirname "$0")" -B "$(dirname "$0")/build/native" -DCMAKE_BUILD_TYPE=Release
cmake --build "$(dirname "$0")/build/native"
"$(dirname "$0")/build/native/openjoey2_smoke"
