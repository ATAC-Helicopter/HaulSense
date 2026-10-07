#!/usr/bin/env bash
# Build dependency only: immutable upstream MIT binary, verified before publication.
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
target="$root/build/ets2la-provider"
mkdir -p -- "$target"
staged=$(mktemp "$target/.provider.XXXXXX")
trap 'rm -f -- "$staged"' EXIT
curl --fail --location --proto '=https' --tlsv1.2 --max-time 90 'https://raw.githubusercontent.com/ETS2LA/ETS2LA/41945bc41e36191fbdf277c7fa6f7dd6f101d679/Assets/SDKs/1.61/Windows/ets2la_plugin.dll' --output "$staged"
actual=$(sha256sum -- "$staged");actual=${actual%% *}
[[ "$actual" == '0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f' ]] || { echo 'Provider checksum mismatch' >&2; exit 1; }
chmod 644 "$staged"
mv -- "$staged" "$target/ets2la_plugin.dll"
