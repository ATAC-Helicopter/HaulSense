#!/usr/bin/env bash
set -euo pipefail
HERE="$(cd "$(dirname "$0")" && pwd)"
cd "$HERE"
command -v clang++ >/dev/null || { echo 'clang++ is required' >&2; exit 1; }
command -v lld-link >/dev/null || { echo 'lld-link is required' >&2; exit 1; }
lld-link /dll /noentry /def:ws2_32.def /out:ws2_32_stub.dll /implib:ws2_32.lib >/dev/null
clang++ --target=x86_64-pc-windows-msvc -c ats_dualsense_plugin_win.cpp \
  -o ats_dualsense_plugin_win.obj -O2 -fno-exceptions -fno-rtti -fno-builtin \
  -fms-extensions -fms-compatibility-version=19.38
lld-link /dll /noentry /machine:x64 /def:plugin.def \
  /out:ats_dualsense_telemetry.dll ats_dualsense_plugin_win.obj ws2_32.lib
rm -f ats_dualsense_plugin_win.obj ws2_32.lib ws2_32_stub.dll
file ats_dualsense_telemetry.dll || true
