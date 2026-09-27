#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
"$ROOT/scripts/install-daemon.sh"
"$ROOT/scripts/install-udev.sh"
"$ROOT/scripts/install-ats-proton.sh"
systemctl --user enable --now ats-dualsense.service
systemctl --user restart ats-dualsense.service

echo
echo "ATS DualSense Bridge v0.7.1 installed and running."
echo "Diagnostics: ats-dualsense --diagnostics"
echo "Live log:    journalctl --user -u ats-dualsense.service -f"
