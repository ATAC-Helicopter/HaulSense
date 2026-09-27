#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CONFIG_DIR="$HOME/.config/ats-dualsense"
CONFIG_FILE="$CONFIG_DIR/config.conf"

# Always regenerate: packaged build caches are intentionally unsupported.
rm -rf "$ROOT/build/daemon"
cmake -S "$ROOT/daemon" -B "$ROOT/build/daemon" -DCMAKE_BUILD_TYPE=Release
cmake --build "$ROOT/build/daemon" -j"$(nproc)"
install -Dm755 "$ROOT/build/daemon/ats-dualsense" "$HOME/.local/bin/ats-dualsense"
mkdir -p "$HOME/.config/systemd/user" "$CONFIG_DIR"
install -m644 "$ROOT/systemd/ats-dualsense.service" "$HOME/.config/systemd/user/ats-dualsense.service"

if [[ ! -f "$CONFIG_FILE" ]]; then
  install -m644 "$ROOT/config/config.conf" "$CONFIG_FILE"
  echo "Installed v0.7.1 default config: $CONFIG_FILE"
elif grep -q '^rumble_strength=0.72$' "$CONFIG_FILE" && \
     grep -q '^road_strength=0.85$' "$CONFIG_FILE" && \
     grep -q '^trigger_strength=0.78$' "$CONFIG_FILE"; then
  cp "$CONFIG_FILE" "$CONFIG_FILE.v0.4.bak"
  install -m644 "$ROOT/config/config.conf" "$CONFIG_FILE"
  echo "Migrated untouched v0.4 defaults to quieter v0.5.1 defaults."
  echo "Previous config backed up to: $CONFIG_FILE.v0.4.bak"
elif grep -q '^rumble_strength=0.24$' "$CONFIG_FILE" && \
     grep -q '^road_strength=0.32$' "$CONFIG_FILE" && \
     grep -q '^left_indicator_mask=0x10$' "$CONFIG_FILE" && \
     grep -q '^right_indicator_mask=0x01$' "$CONFIG_FILE"; then
  cp "$CONFIG_FILE" "$CONFIG_FILE.v0.5.bak"
  install -m644 "$ROOT/config/config.conf" "$CONFIG_FILE"
  echo "Migrated untouched v0.5 defaults to v0.5.1 sequential indicator defaults."
  echo "Previous config backed up to: $CONFIG_FILE.v0.5.bak"
elif grep -q '^rumble_strength=0.24$' "$CONFIG_FILE" && \
     grep -q '^road_strength=0.32$' "$CONFIG_FILE" && \
     grep -q '^left_indicator_inner=0x08$' "$CONFIG_FILE" && \
     grep -q '^right_indicator_inner=0x02$' "$CONFIG_FILE"; then
  cp "$CONFIG_FILE" "$CONFIG_FILE.v0.5.1.bak"
  install -m644 "$ROOT/config/config.conf" "$CONFIG_FILE"
  echo "Migrated untouched v0.5.1 defaults to v0.6 exact-LED/quieter defaults."
  echo "Previous config backed up to: $CONFIG_FILE.v0.5.1.bak"
elif grep -q '^rumble_strength=0.18$' "$CONFIG_FILE" && \
     grep -q '^road_strength=0.25$' "$CONFIG_FILE" && \
     grep -q '^trigger_strength=0.58$' "$CONFIG_FILE" && \
     grep -q '^left_indicator_inner=0x08$' "$CONFIG_FILE" && \
     grep -q '^right_indicator_inner=0x02$' "$CONFIG_FILE"; then
  cp "$CONFIG_FILE" "$CONFIG_FILE.v0.6.bak"
  install -m644 "$ROOT/config/config.conf" "$CONFIG_FILE"
  echo "Migrated untouched v0.6 defaults to v0.7.1 tuned defaults."
  echo "Previous config backed up to: $CONFIG_FILE.v0.6.bak"
else
  echo "Kept customized config: $CONFIG_FILE"
  echo "New reference defaults: $ROOT/config/config.conf"
fi

systemctl --user daemon-reload
if systemctl --user is-enabled --quiet ats-dualsense.service 2>/dev/null || systemctl --user is-active --quiet ats-dualsense.service 2>/dev/null; then
  systemctl --user enable --now ats-dualsense.service
  systemctl --user restart ats-dualsense.service
  echo "Updated and restarted ats-dualsense.service"
else
  echo "Installed native daemon: $HOME/.local/bin/ats-dualsense"
  echo "Enable automatic startup with: systemctl --user enable --now ats-dualsense.service"
fi

echo "For exact left/right player LEDs, keep the udev rule installed (run once if needed): $ROOT/scripts/install-udev.sh"
