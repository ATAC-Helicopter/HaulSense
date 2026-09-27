#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG_DIR="${XDG_CONFIG_HOME:-$HOME/.config}/ats-dualsense"
if [[ -x "$ROOT/bin/haulsense" && "${BUILD_FROM_SOURCE:-0}" != 1 ]] && "$ROOT/bin/haulsense" --version >/dev/null 2>&1; then
  BINARY="$ROOT/bin/haulsense"
else
  cmake -S "$ROOT/daemon" -B "$ROOT/build/daemon" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build/daemon" -j"${BUILD_JOBS:-2}"
  BINARY="$ROOT/build/daemon/haulsense"
fi
systemctl --user stop ats-dualsense.service haulsense.service 2>/dev/null || true
systemctl --user disable ats-dualsense.service 2>/dev/null || true
install -Dm755 "$BINARY" "$HOME/.local/bin/haulsense"
# Compatibility for existing scripts and diagnostics.
ln -sfn haulsense "$HOME/.local/bin/ats-dualsense"
mkdir -p "$HOME/.config/systemd/user" "$CONFIG_DIR"
install -m644 "$ROOT/systemd/haulsense.service" "$HOME/.config/systemd/user/haulsense.service"
if [[ ! -f "$CONFIG_DIR/config.conf" ]]; then
  install -m644 "$ROOT/config/config.conf" "$CONFIG_DIR/config.conf"
else
  cp -n "$CONFIG_DIR/config.conf" "$CONFIG_DIR/config.conf.pre-haulsense.bak" || true
  echo "Preserved your customized settings: $CONFIG_DIR/config.conf"
fi
install -Dm644 "$ROOT/ui/haulsense.desktop" "$HOME/.local/share/applications/haulsense.desktop"
install -Dm644 "$ROOT/ui/haulsense.svg" "$HOME/.local/share/icons/hicolor/scalable/apps/haulsense.svg"
systemctl --user daemon-reload
echo "Installed HaulSense. Dashboard: http://127.0.0.1:39056"
