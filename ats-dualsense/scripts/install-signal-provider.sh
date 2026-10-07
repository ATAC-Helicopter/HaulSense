#!/usr/bin/env bash
# Optional, separate upstream memory provider, pinned to the verified 1.61 build.
set -euo pipefail
if [[ "${GAME_VERSION:-}" != '1.61' ]]; then
  echo 'This provider is pinned for ATS/ETS2 1.61. Set GAME_VERSION=1.61 after checking your game version.' >&2
  exit 2
fi
game="${ATS_DIR:-/mnt/games/SteamLibrary/steamapps/common/American Truck Simulator}"
if [[ ! -d "$game/bin/win_x64" ]]; then echo 'Set ATS_DIR to the Windows game installation.' >&2; exit 2; fi
url='https://raw.githubusercontent.com/ETS2LA/ETS2LA/41945bc41e36191fbdf277c7fa6f7dd6f101d679/Assets/SDKs/1.61/Windows/ets2la_plugin.dll'
checksum='0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f'
plugins="$game/bin/win_x64/plugins";mkdir -p -- "$plugins"
staged=$(mktemp "$plugins/.haulsense-provider.XXXXXX")
trap 'rm -f -- "$staged"' EXIT
curl --fail --location --proto '=https' --tlsv1.2 --max-time 90 "$url" --output "$staged"
actual=$(sha256sum -- "$staged");actual=${actual%% *}
if [[ "$actual" != "$checksum" ]]; then echo 'Provider checksum mismatch; nothing installed.' >&2; exit 1; fi
if [[ -f "$plugins/ets2la_plugin.dll" ]]; then cp -p -- "$plugins/ets2la_plugin.dll" "$plugins/ets2la_plugin.dll.bak-$(date -u +%Y%m%d-%H%M%S)"; fi
chmod 644 "$staged";mv -- "$staged" "$plugins/ets2la_plugin.dll"
echo 'Installed the pinned ETS2LA 1.61 provider. Restart the game to expose semaphore and route feeds.'
echo 'Upstream MIT plugin license: https://github.com/dariowouters/ets2la_plugin/blob/c925bd965a96c58b73eaea9ca033fe0f0dc623a7/LICENSE.md'
