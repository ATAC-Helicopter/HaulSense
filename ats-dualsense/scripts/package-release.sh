#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${RELEASE_VERSION:-$(tr -d '\r\n' < "$ROOT/VERSION")}"
[[ "$VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+(-[A-Za-z0-9.]+)?$ ]] || { echo "Invalid release version" >&2; exit 2; }
STAGE="$ROOT/build/release/HaulSense-$VERSION"
# STAGE is a version-validated, generated path below build/release.
# A repeated package must not nest a second standalone bundle.
rm -rf -- "$STAGE"
mkdir -p "$STAGE/ats-dualsense/bin"
for file in README.md LICENSE CHANGELOG.md CONTRIBUTING.md SECURITY.md CMakeLists.txt VERSION; do install -m644 "$ROOT/$file" "$STAGE/$file"; done
cp -r "$ROOT/docs" "$ROOT/tests" "$STAGE/"
for part in config daemon plugin-win scripts systemd third_party ui; do
  mkdir -p "$STAGE/ats-dualsense/$part"
  cp -r "$ROOT/ats-dualsense/$part/." "$STAGE/ats-dualsense/$part/"
done
mkdir -p "$STAGE/ats-dualsense/desktop"
for file in main.cjs policy.cjs maps.cjs embed-maps.cjs package.cjs package.json package-lock.json; do install -m644 "$ROOT/ats-dualsense/desktop/$file" "$STAGE/ats-dualsense/desktop/$file"; done
cp -r "$ROOT/ats-dualsense/desktop/test" "$STAGE/ats-dualsense/desktop/"
mkdir -p "$STAGE/scripts"
cp -r "$ROOT/scripts/." "$STAGE/scripts/"
if [[ -x "$ROOT/build/desktop/HaulSense-linux-x64/haulsense-app" ]]; then cp -r "$ROOT/build/desktop/HaulSense-linux-x64" "$STAGE/standalone"; fi
# Game-derived packs belong to the local installation, never the public archive.
rm -rf -- "$STAGE/standalone/resources/maps"
find "$STAGE" -type d -name __pycache__ -prune -exec rm -rf -- {} +
install -m755 "$ROOT/build/ats-dualsense/daemon/haulsense" "$STAGE/ats-dualsense/bin/haulsense"
rm -f "$STAGE/ats-dualsense/plugin-win/"*.lib "$STAGE/ats-dualsense/plugin-win/"*.obj "$STAGE/ats-dualsense/plugin-win/"*_stub.dll
rm -f "$STAGE/ats-dualsense/systemd/ats-dualsense.service"
ARCHIVE="haulsense-$VERSION-linux-x86_64.tar.gz"
tar -czf "$ROOT/build/release/$ARCHIVE" -C "$ROOT/build/release" "HaulSense-$VERSION"
(cd "$ROOT/build/release" && sha256sum "$ARCHIVE" > "$ARCHIVE.sha256")
echo "$ROOT/build/release/$ARCHIVE"
