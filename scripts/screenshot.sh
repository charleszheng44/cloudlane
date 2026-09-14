#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"
mkdir -p build/screenshot
qmake6 -o build/screenshot/Makefile tests/screenshot.pro
make -C build/screenshot -j"${CLOUDLANE_JOBS:-4}" > build/screenshot/build.log 2>&1
./build/screenshot/screenshot "${1:-build/cloudlane-preview.png}"
