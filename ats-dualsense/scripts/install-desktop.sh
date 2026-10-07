#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
app_dir="$root/build/desktop/HaulSense-linux-x64"
if [[ ! -x "$app_dir/haulsense-app" ]]; then
  npm ci --prefix "$root/ats-dualsense/desktop" --ignore-scripts --no-audit --no-fund
  node "$root/ats-dualsense/desktop/node_modules/electron/install.js"
  npm run --prefix "$root/ats-dualsense/desktop" package
fi
install_dir="${XDG_DATA_HOME:-$HOME/.local/share}/haulsense/desktop"
mkdir -p -- "$(dirname -- "$install_dir")" "$HOME/.local/bin"
if [[ -d "$install_dir" ]]; then mv -- "$install_dir" "$install_dir.backup-$(date -u +%Y%m%d-%H%M%S)"; fi
cp -a -- "$app_dir" "$install_dir"
ln -sfn -- "$install_dir/haulsense-app" "$HOME/.local/bin/haulsense-app"
install -Dm644 "$root/ats-dualsense/ui/haulsense.desktop" "$HOME/.local/share/applications/haulsense.desktop"
install -Dm644 "$root/ats-dualsense/ui/haulsense.svg" "$HOME/.local/share/icons/hicolor/scalable/apps/haulsense.svg"
echo "Installed standalone HaulSense. Launch haulsense-app."
