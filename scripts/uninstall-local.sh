#!/usr/bin/env bash
set -euo pipefail
install_prefix="${CLOUDLANE_PREFIX:-${YUNJIAN_PREFIX:-$HOME/.local}}"
desktop_id=io.github.charleszheng44.Cloudlane
rm -f -- "$install_prefix/bin/cloudlane" \
  "$install_prefix/share/applications/$desktop_id.desktop" \
  "$install_prefix/share/icons/hicolor/scalable/apps/$desktop_id.svg" \
  "$install_prefix/share/licenses/cloudlane/LICENSE" \
  "$install_prefix/share/licenses/cloudlane/API-Enhanced-LICENSE"
if [[ -L "$install_prefix/bin/yunjian" && "$(readlink -- "$install_prefix/bin/yunjian")" == cloudlane ]]; then
  rm -- "$install_prefix/bin/yunjian"
fi
if command -v update-desktop-database >/dev/null; then
  update-desktop-database "$install_prefix/share/applications"
fi
printf 'Removed Cloudlane from %s. Your account, library and downloads are retained.\n' "$install_prefix"
