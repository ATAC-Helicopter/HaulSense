#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
DLL="$ROOT/plugin-win/ats_dualsense_telemetry.dll"

if [[ ! -f "$DLL" ]]; then
  echo "Missing prebuilt DLL: $DLL" >&2
  exit 1
fi

GAME="${ATS_DIR:-}"
if [[ -z "$GAME" ]]; then
  candidates=(
    "/mnt/games/SteamLibrary/steamapps/common/American Truck Simulator"
    "$HOME/.local/share/Steam/steamapps/common/American Truck Simulator"
    "$HOME/.steam/steam/steamapps/common/American Truck Simulator"
  )
  for c in "${candidates[@]}"; do
    if [[ -d "$c/bin/win_x64" ]]; then GAME="$c"; break; fi
  done
fi

if [[ -z "$GAME" || ! -d "$GAME/bin/win_x64" ]]; then
  echo "ATS Windows/Proton installation not found automatically." >&2
  echo "Run: ATS_DIR='/path/to/American Truck Simulator' $0" >&2
  exit 2
fi

PLUGINS="$GAME/bin/win_x64/plugins"
mkdir -p "$PLUGINS"
# Atomic replacement preserves the old mapped image while a running game exits.
DEST="$PLUGINS/ats_dualsense_telemetry.dll"
if [[ -f "$DEST" ]]; then cp -n "$DEST" "$DEST.pre-haulsense.bak" || true; fi
STAGED="$(mktemp "$PLUGINS/.haulsense-plugin.XXXXXX")"
trap 'rm -f "$STAGED"' EXIT
install -m644 "$DLL" "$STAGED"
mv -f "$STAGED" "$DEST"

echo "Installed ATS telemetry plugin:"
echo "  $PLUGINS/ats_dualsense_telemetry.dll"
