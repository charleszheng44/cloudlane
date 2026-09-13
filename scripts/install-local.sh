#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
install_prefix="${YUNJIAN_PREFIX:-$HOME/.local}"
mkdir -p "$repo_dir/build"
qmake6 -o "$repo_dir/build/Makefile" "$repo_dir/yunjian.pro"
make -C "$repo_dir/build" -j"${YUNJIAN_JOBS:-4}"
install -Dm755 "$repo_dir/build/yunjian" "$install_prefix/bin/yunjian.new"
mv -f -- "$install_prefix/bin/yunjian.new" "$install_prefix/bin/yunjian"
install -Dm644 "$repo_dir/packaging/io.github.charleszheng44.Yunjian.desktop" "$install_prefix/share/applications/io.github.charleszheng44.Yunjian.desktop"
install -Dm644 "$repo_dir/packaging/io.github.charleszheng44.Yunjian.svg" "$install_prefix/share/icons/hicolor/scalable/apps/io.github.charleszheng44.Yunjian.svg"
if command -v update-desktop-database >/dev/null; then update-desktop-database "$install_prefix/share/applications"; fi
printf 'Installed 云间 to %s/bin/yunjian\n' "$install_prefix"
