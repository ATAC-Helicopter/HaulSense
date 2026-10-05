#!/usr/bin/env bash
set -euo pipefail
systemctl --user disable --now haulsense.service ats-dualsense.service 2>/dev/null || true
systemctl --user stop haulsense-hud.service 2>/dev/null || true
rm -f "$HOME/.local/bin/haulsense" "$HOME/.local/bin/ats-dualsense" "$HOME/.local/bin/haulsense-hud"
rm -f "$HOME/.config/systemd/user/haulsense.service" "$HOME/.config/systemd/user/ats-dualsense.service"
rm -f "$HOME/.local/share/applications/haulsense.desktop" "$HOME/.local/share/applications/haulsense-hud.desktop" "$HOME/.local/share/icons/hicolor/scalable/apps/haulsense.svg"
systemctl --user daemon-reload
candidates=("${ATS_DIR:-}" "/mnt/games/SteamLibrary/steamapps/common/American Truck Simulator" "$HOME/.local/share/Steam/steamapps/common/American Truck Simulator" "$HOME/.steam/steam/steamapps/common/American Truck Simulator")
for c in "${candidates[@]}"; do
  [[ -n "$c" ]] || continue
  rm -f "$c/bin/win_x64/plugins/ats_dualsense_telemetry.dll"
done
echo "Removed HaulSense service, binary, app launcher and plugin. Kept config and udev rule."
