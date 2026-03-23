#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
build_dir="${1:-$repo_root/build/metalcraft}"
jobs="${JOBS:-$(nproc)}"

cmake -S "$repo_root/Natives/renderer" -B "$build_dir" -DMETALCRAFT_BUILD_TESTS=ON
cmake --build "$build_dir" -j"$jobs"
ctest --test-dir "$build_dir" --output-on-failure
