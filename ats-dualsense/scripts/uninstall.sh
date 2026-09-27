#!/usr/bin/env bash
set -euo pipefail
systemctl --user disable --now ats-dualsense.service 2>/dev/null || true
rm -f "$HOME/.local/bin/ats-dualsense"
rm -f "$HOME/.config/systemd/user/ats-dualsense.service"
systemctl --user daemon-reload

candidates=(
  "${ATS_DIR:-}"
  "/mnt/games/SteamLibrary/steamapps/common/American Truck Simulator"
  "$HOME/.local/share/Steam/steamapps/common/American Truck Simulator"
  "$HOME/.steam/steam/steamapps/common/American Truck Simulator"
)
for c in "${candidates[@]}"; do
  [[ -n "$c" ]] || continue
  if [[ -f "$c/bin/win_x64/plugins/ats_dualsense_telemetry.dll" ]]; then
    rm -f "$c/bin/win_x64/plugins/ats_dualsense_telemetry.dll"
    echo "Removed ATS telemetry plugin from: $c"
    break
  fi
done

echo "Removed daemon/service/plugin."
echo "Kept user config at: $HOME/.config/ats-dualsense/config.conf"
echo "Kept udev rule at: /etc/udev/rules.d/99-ats-dualsense.rules"
