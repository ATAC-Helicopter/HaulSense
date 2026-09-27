# HaulSense

**Feel the long haul.** A small native Linux companion for American Truck Simulator under Proton, with a live truck dashboard and telemetry-driven DualSense effects.

[Project page](https://fglabs.dev/projects/haulsense) · [Releases](https://github.com/FGLabs-dev/HaulSense/releases) · [Report a bug](https://github.com/FGLabs-dev/HaulSense/issues)

> Public alpha: automatic tests cover the SDK adapter, wire protocol, effect engine, local API and UI DOM. Full in-game driving qualification and visual browser review of this new dashboard are still pending. Feedback uses compatible rumble and adaptive triggers; USB audio haptics are not implemented.

## What it does

- **A cockpit that earns its space:** speed, displayed gear, RPM, cruise, route/ETA, fuel consumption/range, brake air, coolant/oil, battery, inputs, wheel contact and component condition. Metric and US units.
- **Directional white LEDs:** left pair and right pair remain separate; each sweeps inner → inner+outer → off. Hazards sweep both; the middle LED follows exterior lights. Hardware calibration can swap the sides.
- **Adaptive L2 brakes**, a subtle R2 full-throttle cue and restrained heavy-brake texture. Setting trigger strength to zero disables all resistance, including low-air cues.
- **Event-driven immersion:** gear shifts, road impacts, engine starts, retarder, engine brake, parking brake, trailer coupling, lift axle and SDK gameplay events. Delivery/fine acknowledgements have distinct colours. Optional gentle reverse and wiper rhythms.
- **Calm lightbar:** blue driving/lights, white reverse, amber hazards, breathing gold beacon. Engine warnings are gated by engine state; critical wear starts at 85%.
- **Fine tuning:** Calm, Balanced and Immersive presets, eight sliders, optional cues, master pause and persistent settings. No continuous RPM/throttle vibration.
- **SDK inspector:** subscribed values and missing-channel status, without pretending unavailable values are zero. Primary-trailer telemetry is included; extended trailer trains and spatial placement are not exposed yet.
- **Local and lightweight:** C++20 daemon, no runtime Node/Electron/Python, no cloud, no third-party scripts/fonts, no account. Dashboard is embedded in the binary, opens in your browser, and stops fetching when hidden. Its 24-second history is bounded and never recorded to disk.

```text
ATS.exe → SCS SDK 1.14 DLL → localhost UDP :39055 → HaulSense → DualSense HID + exact sysfs LEDs
                                                   └→ localhost HTTP :39056 → browser cockpit
```

## Install

Restart ATS after installation to load the updated plugin. Linux x86-64, DualSense/DualSense Edge and the kernel `hid-playstation` driver are required. USB and Bluetooth are supported by the report encoder; broad firmware/transport qualification remains an alpha gate.

Source prerequisites: GCC 11+ or Clang with C++20 support, CMake 3.20+, and Clang + LLD for the Windows plugin. The plugin is freestanding: no Windows SDK or MinGW is needed. Release bundles include the DLL and an optional Linux executable, so Clang/LLD are not needed to install a bundle.

```bash
./ats-dualsense/scripts/install-all.sh
```

Game discovery checks common Steam library locations. Override it with:

```bash
ATS_DIR='/path/to/American Truck Simulator' ./ats-dualsense/scripts/install-all.sh
```

The installer installs the DLL, user service, app launcher and udev permissions. It preserves customized config and backs it up before migration. Udev installation is the only step requiring sudo. If suitable permissions already exist, use `SKIP_UDEV=1`.

Open **HaulSense** from the application menu, or visit **http://127.0.0.1:39056**. `haulsense.service` runs independently of the dashboard. The old `ats-dualsense` executable becomes a compatibility alias; the old service is disabled to avoid two bridges fighting over the controller.

## Build / test

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j2
ctest --test-dir build --output-on-failure
python3 tests/integration.py build
./ats-dualsense/plugin-win/build-windows-dll.sh
```

Optional DOM smoke test (test dependency only):

```bash
npm install --prefix .test-deps linkedom --no-audit --no-fund
NODE_PATH="$PWD/.test-deps/node_modules" node tests/ui.mjs build/ui-state.json
```

**Hardware-free preview:** stop the installed service, run `./build/ats-dualsense/daemon/haulsense --mock --config /tmp/haulsense-demo.conf`, and open the dashboard. Demo state is clearly labelled and never opens the controller. `--mock-hardware` explicitly opts into controller output.

## Control and diagnostics

```bash
haulsense --diagnostics
haulsense --telemetry-debug
journalctl --user -u haulsense.service -f
systemctl --user stop haulsense.service
haulsense --led-test
systemctl --user start haulsense.service
```

Stop the service before launching another daemon instance or an LED test. LED discovery is associated with the selected HID device, including when several controllers are connected. Live player LED control deliberately uses sysfs: raw HID player patterns can differ by firmware. The diagnostic lights each physical LED separately.

Settings remain in `~/.config/ats-dualsense/config.conf` for migration compatibility, or `$XDG_CONFIG_HOME/ats-dualsense/config.conf` when set. `--config PATH` overrides the location. The dashboard saves only known settings and preserves other user keys.

Pause events immediately send a neutral state. Missing/invalid telemetry times out after 500 ms. Legacy-plugin pauses use this timeout; the new plugin sends pause immediately. Frames must match an exact supported packet version and size; the legacy v4/175-byte adapter keeps an already running game compatible during migration, with inferred availability clearly labelled. New v5 frames carry explicit channel availability; bad frames are counted, not applied. The socket is drained to the newest valid frame, with bounded per-loop work. Controller output is capped at 50 Hz; changed LEDs/HID output are sent only when necessary.

[Architecture and security boundaries](docs/ARCHITECTURE.md) · [SDK coverage](docs/TELEMETRY.md) · [Qualification](docs/QUALIFICATION.md) · [Roadmap](docs/ROADMAP.md)

## Remove

```bash
./ats-dualsense/scripts/uninstall.sh
```

Keeps user settings and the udev rule. Third-party SDK notices are in `ats-dualsense/third_party/scs-sdk/LICENSE`. HaulSense is an independent FG Labs project, unaffiliated with Sony or SCS Software.
