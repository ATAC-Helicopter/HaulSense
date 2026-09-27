#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
sudo install -m644 "$ROOT/99-ats-dualsense.rules" /etc/udev/rules.d/99-ats-dualsense.rules
sudo udevadm control --reload-rules
sudo udevadm trigger --subsystem-match=leds || true
sudo udevadm trigger --subsystem-match=hidraw || true
echo "Installed DualSense permissions. Reconnect the controller once, then run:"
echo "  ats-dualsense --diagnostics"
