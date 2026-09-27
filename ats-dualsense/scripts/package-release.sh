#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
VERSION="${RELEASE_VERSION:-0.8.0-alpha.1}"
STAGE="$ROOT/build/release/HaulSense-$VERSION"
mkdir -p "$STAGE/ats-dualsense/bin"
for file in README.md LICENSE CHANGELOG.md CONTRIBUTING.md SECURITY.md CMakeLists.txt; do install -m644 "$ROOT/$file" "$STAGE/$file"; done
cp -r "$ROOT/docs" "$ROOT/tests" "$STAGE/"
for part in config daemon plugin-win scripts systemd third_party ui; do
  mkdir -p "$STAGE/ats-dualsense/$part"
  cp -r "$ROOT/ats-dualsense/$part/." "$STAGE/ats-dualsense/$part/"
done
install -m755 "$ROOT/build/ats-dualsense/daemon/haulsense" "$STAGE/ats-dualsense/bin/haulsense"
rm -f "$STAGE/ats-dualsense/plugin-win/"*.lib "$STAGE/ats-dualsense/plugin-win/"*.obj "$STAGE/ats-dualsense/plugin-win/"*_stub.dll
rm -f "$STAGE/ats-dualsense/systemd/ats-dualsense.service"
ARCHIVE="haulsense-$VERSION-linux-x86_64.tar.gz"
tar -czf "$ROOT/build/release/$ARCHIVE" -C "$ROOT/build/release" "HaulSense-$VERSION"
(cd "$ROOT/build/release" && sha256sum "$ARCHIVE" > "$ARCHIVE.sha256")
echo "$ROOT/build/release/$ARCHIVE"
