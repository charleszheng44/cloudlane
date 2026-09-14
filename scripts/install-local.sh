#!/usr/bin/env bash
set -euo pipefail
repo_dir="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)"
install_prefix="${CLOUDLANE_PREFIX:-${YUNJIAN_PREFIX:-$HOME/.local}}"
mkdir -p "$repo_dir/build"
qmake6 -o "$repo_dir/build/Makefile" "$repo_dir/cloudlane.pro"
make -C "$repo_dir/build" -j"${CLOUDLANE_JOBS:-${YUNJIAN_JOBS:-4}}"
install -Dm755 "$repo_dir/build/cloudlane" "$install_prefix/bin/cloudlane.new"
mv -f -- "$install_prefix/bin/cloudlane.new" "$install_prefix/bin/cloudlane"
install -Dm644 "$repo_dir/packaging/io.github.charleszheng44.Cloudlane.desktop" "$install_prefix/share/applications/io.github.charleszheng44.Cloudlane.desktop"
install -Dm644 "$repo_dir/packaging/io.github.charleszheng44.Cloudlane.svg" "$install_prefix/share/icons/hicolor/scalable/apps/io.github.charleszheng44.Cloudlane.svg"
install -Dm644 "$repo_dir/LICENSE" "$install_prefix/share/licenses/cloudlane/LICENSE"
install -Dm644 "$repo_dir/third_party/API-Enhanced-LICENSE" "$install_prefix/share/licenses/cloudlane/API-Enhanced-LICENSE"
# Migrate this project's old launcher, preserving saved account and data.
old_id=io.github.charleszheng44.Yunjian
if [[ -f "$install_prefix/share/applications/$old_id.desktop" ]]; then
  rm -f -- "$install_prefix/share/applications/$old_id.desktop" "$install_prefix/share/icons/hicolor/scalable/apps/$old_id.svg"
  ln -sfn -- cloudlane "$install_prefix/bin/yunjian"
fi
if command -v update-desktop-database >/dev/null; then update-desktop-database "$install_prefix/share/applications"; fi
printf 'Installed Cloudlane to %s/bin/cloudlane\n' "$install_prefix"
