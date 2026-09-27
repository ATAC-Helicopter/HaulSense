#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
# Build and locate the game before changing any installed service.
if [[ ! -f "$ROOT/plugin-win/ats_dualsense_telemetry.dll" ]]; then
  "$ROOT/plugin-win/build-windows-dll.sh"
fi
if [[ ! -x "$ROOT/bin/haulsense" || "${BUILD_FROM_SOURCE:-0}" == 1 ]] || ! "$ROOT/bin/haulsense" --version >/dev/null 2>&1; then
  cmake -S "$ROOT/daemon" -B "$ROOT/build/daemon" -DCMAKE_BUILD_TYPE=Release
  cmake --build "$ROOT/build/daemon" -j"${BUILD_JOBS:-2}"
fi
"$ROOT/scripts/install-ats-proton.sh"
"$ROOT/scripts/install-daemon.sh"
if [[ "${SKIP_UDEV:-0}" != 1 ]]; then "$ROOT/scripts/install-udev.sh"; fi
systemctl --user enable --now haulsense.service
echo "HaulSense installed. Open the HaulSense app or http://127.0.0.1:39056"
echo "Diagnostics: haulsense --diagnostics"
