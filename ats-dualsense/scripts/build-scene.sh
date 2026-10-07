#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/../.." && pwd)
deps=$(mktemp -d -t haulsense-scene.XXXXXX)
trap 'rm -rf -- "$deps"' EXIT
npm install --prefix "$deps" --ignore-scripts --no-audit --no-fund three@0.180.0 esbuild@0.25.6
"$deps/node_modules/.bin/esbuild" "$root/ats-dualsense/ui/scene.js" --bundle --minify --format=iife --outfile="$root/ats-dualsense/ui/scene.bundle.js" --alias:three="$deps/node_modules/three/build/three.module.js"
