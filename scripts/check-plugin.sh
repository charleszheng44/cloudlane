#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"
omarchy plugin validate .
mkdir -p build/plugin-test/plugin build/mpris-test
cp plugin/BarWidget.qml build/plugin-test/plugin/
cp tests/plugin-smoke.qml build/plugin-test/shell.qml
qmake6 -o build/mpris-test/Makefile tests/mpris_test.pro
make -C build/mpris-test -j"${CLOUDLANE_JOBS:-4}" > build/mpris-test/build.log 2>&1
# A private bus ensures the check cannot operate the user's live players.
CLOUDLANE_PLUGIN_TEST=1 QT_QPA_PLATFORM=offscreen QT_QUICK_BACKEND=software \
  dbus-run-session --config-file=tests/dbus-session.conf -- ./build/mpris-test/mpris-test
