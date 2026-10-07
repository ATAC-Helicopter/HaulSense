# Qualification evidence — 1.0.0-rc.1

## Candidate checks (2026-10-07)

- Six native Release tests and six UndefinedBehaviorSanitizer tests pass, including the semaphore ABI, route reader, typed delivery events and durable journal. The sanitizer found an unaligned packed-field reference in journal serialization; numeric values now copy into aligned JSON values.
- Real loopback HTTP/UDP integration verifies protocol v4/v5/v6/v7, job completion, saved report retrieval, input rejection and HTTP origin controls. DOM tests cover dashboard, HUD, map, routing and reports.
- Electron 44.6.0 was launched with its renderer sandbox and Node disabled. In an isolated DEMO profile, an 82 MiB local map survived reload and complete application restart. The daemon sequence advanced while the window was minimized. This verifies background progress, not a measured long-haul frame-rate guarantee.
- The actual packaged, installed application was also launched without `--no-sandbox`; its local map restored automatically and Node remained unavailable in the renderer. The Playwright Electron harness adds test launch flags, so renderer preferences alone were not treated as installed sandbox proof.
- The rendered job report and native JSON save dialog were exercised. Report values are synthetic and the illustrated path is sampled from imported roads; the screenshot does not represent a driven job. No renderer JavaScript errors were observed.
- Desktop npm audit reports zero vulnerabilities; dependency lock and Electron fuses restrict development entry points. Repository secret scanning and push protection are enabled. Redacted Gitleaks scans of Git history and the selected release changes found no secrets.
- The optional provider is pinned to the ETS2LA 1.61 DLL by immutable source URL and SHA-256. Reader tests verify state codes, world cells and expiry. Actual semaphore transitions and a complete delivery with the newly installed plugins still require a game restart and live driving.

Optional rendered checks are reproducible with Playwright available through `NODE_PATH`: `node tests/desktop.cjs <local scene.json>` and `node tests/desktop-report.cjs <local scene.json>`. The map is private game-derived input and is not included in CI.

GitHub CodeQL initially reported missing worker origin checks and untrusted graph indexes. The worker now checks origins and validates numeric endpoint indexes; regression fixtures cover foreign messages and malformed edges. Packaging also clears its generated staging directory to prevent duplicate bundles on repeated runs.

## Stable release gates

- Real ATS 1.61 and ETS2 qualification: mapped position, fresh signal transitions, GPS route matching, delivery/cancellation, report restart recovery and controller behavior.
- Sustained foreground/background performance and memory measurements with a full scene during driving.
- Review and pass remote build/security checks before publishing stable 1.0. No stable release is claimed by this candidate.

## Historical 0.8 evidence


## Passed locally

- Native GCC 13 Release build with LTO/size optimization and strict warnings.
- Undefined-behavior sanitizer checks packed callbacks, packet validation and snapshot access. Scalars/vectors copy as bytes into packed wire fields; no unaligned scalar pointers escape the decoder.
- CTest effects regression: separate left/right/hazard masks, side swap, center light, pause/master mute, zero-strength triggers, steady-terrain silence, finite wire validation and settings preservation.
- Native harness compiles the exact plugin with official SDK headers and exercises callback values/unavailability, truck config, 50 Hz ceiling and pause/resume.
- Real loopback HTTP/UDP integration: malformed/truncated/oversized/version-mismatched packets, directional output, pause/timeout, validated settings, Host/CSRF rejection and slow-client isolation. Tests use isolated ports so real ATS traffic cannot contaminate fixtures.
- Dashboard DOM smoke: demo labels, wheels, eight sliders, optional cues, full inspector, US unit conversions and stale-data clearing. This is DOM evidence, not rendered browser evidence.
- Clang 18 / LLD 18 produce a freestanding x64 PE DLL importing only ws2_32.dll and kernel32.dll.
- Actual attached USB DualSense discovered on hidraw5, independently writable player-1 through player-5 associated with the same HID device. The actual six-step LED test read back exactly one selected LED for masks 0x10, 0x08, 0x04, 0x02, 0x01, then all off for 0x00. This verifies independent driver output, not the user-visible physical orientation during driving.
- FG Labs site ESLint and production Next.js build pass, including the new product page.

## Bounded performance sample

Local 5-second samples on this workstation, Linux daemon only; no controller writes in these simulations. CPU is percent of one logical core, quantized by kernel accounting. Browser RSS/CPU is separate. These short samples are not a long-haul/game benchmark.

| Scenario | CPU / core | RSS |
|---|---:|---:|
| idle | 0.0% | 3416 KiB |
| demo | 0.2% | 3552 KiB |
| demo_with_dashboard | 0.2% | 3804 KiB |

Pre-migration-adapter native binary: 183,608 bytes. The adapter adds a small amount of code; release artifact sizes are authoritative. Dashboard asset is about 33 KiB and is embedded, not a runtime dependency.

## Still open before stable

- Rendered desktop/mobile browser review: the installed Browser plugin cannot bootstrap because it references a missing browser-service path. No screenshot or rendered layout claim is made.
- Complete new-plugin real driving matrix: directional LEDs, hazards, reverse, road/brake tuning, truck changes, pause/resume, jobs and persistence.
- Transport/firmware matrix including Bluetooth, DualSense Edge and reconnects.
- Longer real-game resource sampling.

The running legacy plugin can be observed during migration, but that is not qualification of the new plugin. Public release stays prerelease until the open gates have evidence.

## Earlier instant player LED investigation

The user reported firmware center-out animation during a single directional signal. The prior sysfs path emits intermediate masks without the firmware instant flag. Gameplay and LED diagnostics now send the complete mask atomically with bit 5 set, through the same output report as the other effects. USB and Bluetooth report-byte tests verify left/right/hazard masks, instant application and neutral clearing. Historical sysfs brightness readback does not validate this new raw HID path. Physical driving confirmation remains required; do not treat report-byte tests as visual proof.

The attached USB controller accepted all six instant-HID diagnostic writes and neutral clearing after installation. Both Steam and HaulSense currently hold read/write descriptors for the selected controller; this is evidence of possible competing output ownership, not proof of a Steam LED overwrite. The installed binary matches the locally tested build.

On 2026-09-30 the per-game Steam Input override for ATS was temporarily changed from **Enable** to **Disable**. The user confirmed that disabling it prevents driving with the current ATS controller configuration, so it was restored to **Enable** and verified in Steam's Controller page. Steam's separate **Player Slot LED** preference was found **On** and changed to **Off** while keeping Steam Input enabled. The user then confirmed that driving controls worked but the physical LED pattern was still wrong. Neither Steam LED preference nor the instant-HID change is a qualified fix; direct DualSense input and HID ownership remain to investigate. ATS loaded the HaulSense 0.8.0 SDK DLL during the test. The compact browser HUD route and optional GTK HUD were installed; the native window was visually checked as a 313×175 transparent, always-above Xwayland window on the selected monitor. A second capture during real ATS driving displayed LIVE, 89 km/h, a 56 km/h limit and the Ford F150 truck from the SDK. The HUD process used about 23–25 MiB of cgroup memory; the native service about 3 MiB before driving. Borderless and exclusive-fullscreen overlay behavior still require real play verification.

Protocol references: [hid-playstation output path](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c) and [dualsensectl instant player LED implementation](https://github.com/nowrep/dualsensectl/blob/main/main.c).

## Earlier HUD and side isolation investigation — 2026-10-05

- Release build and all three CTest suites passed; all three suites also passed with undefined-behavior sanitizer. SDK harness now checks official job city/mass attributes and empty-job metadata clearing.
- Real HTTP/UDP integration passed. Dashboard and new HUD DOM tests passed, including GPS without a job, job city, null channels, no route, units, hours/minutes, actual lamp phase, hazards, warnings, pause and demo labels. HUD regression is included in CI; remote CI has not been run in this session.
- GTK rendered screenshots reviewed at scale 0.8 and 1.2, including long city names and all warnings; centered text, round limit and warning wrapping are visible. Web HUD rendered in isolated headless Firefox using a demo fixture. These supersede the earlier missing-render evidence for the compact HUD only, not for the complete dashboard/mobile matrix. Generated review images: `build/hud-review/`.
- Native HUD HTTP requests now run on a background thread; GTK never waits on the HTTP timeout. Existing monitor, position, scale, opacity and units configuration was preserved.
- Controller masks exclude the center during any logical directional signal; custom masks cannot illuminate the opposite pair. Player-only report tests assert that trigger/rumble/RGB control flags stay clear. USB/Bluetooth report byte tests pass; physical correctness and competing Steam Input output still need user confirmation.
- A bounded 10 Hz player-only refresh limits stale masks from competing writers; this is not a guarantee of exclusive controller ownership. Full reports still update only when feedback changes. Idle/paused/master-muted operation does not perform this periodic refresh.
- Installed daemon hash matches the tested Release binary. Installed daemon and GTK HUD were restarted without restarting ATS or changing Steam Input. Live game API reports v5, USB controller present and zero rejected packets, with GPS distance/ETA but an empty job destination. During the short sampled driving window no directional signal occurred, so that sample does not qualify the physical indicator fix.
- Installed service memory sample: daemon about 2.2 MiB, HUD about 25.4 MiB (cgroup accounting, short observation, not a long-session performance benchmark).
- No wire ABI or game-plugin behavior change was needed for this update. Freestanding DLL rebuild was unavailable on this session's PATH because clang/lld are absent; the running installed v5 plugin remains compatible.

### Physical failure confirmed in the same session

The user subsequently reported all four outer LEDs still animating. A live left-only capture showed logical left=true, right=false, hazards=false and computed masks 0x08/0x18/0x00; therefore ATS direction and HaulSense side selection were correct in that capture. With HaulSense stopped, a separate player-only USB writer held 0x38 (left pair + instant) for 8 seconds and 0x23 (right pair + instant) for 8 seconds at 50 Hz. The user still saw all four outer LEDs. The periodic refresh is therefore **not** a qualified fix, even at 50 Hz. Steam and HaulSense hold the same hidraw device; Steam's controller-specific `player_slot_led` is already 0. This establishes shared access, not yet proof of the actual competing writer. Do not claim the indicator issue resolved. Controller firmware feature report dates the build to 2025-07-04.

### Isolation result and final implementation

Further user-assisted physical tests in this session:

- A single outer-LED command per side, with and without the instant bit, was reported as a correct single LED.
- Single-command adjacent pairs were still reported as all four LEDs.
- A single-LED inner-to-outer sequence was also reported as both sides simultaneously. This experimental fallback was reverted; it did not qualify the requested behavior.
- With the user's explicit confirmation that ATS was parked and paused, Steam was suspended for a bounded direct-HID pair test, then resumed. The user still reported all four. A separate systemd timer guaranteed automatic resume; Steam and the original telemetry service were restored afterwards.
- The equivalent test through the Linux kernel's five player brightness interfaces, again with Steam suspended and HaulSense stopped, read back `[0,0,0,1,1]` for left and `[1,1,0,0,0]` for right. The user still reported all four physically. The discrepancy persists outside HaulSense and with the Steam client suspended. Only one Sony DualSense HID device is present. This does not establish a specific firmware/hardware defect; direct visual evidence is needed before claiming that cause.

At this stage, the code retained inner-to-pair side masks and suppresses the center during any directional request. It sends the player-control flag only when the player mask changes; RGB/rumble/trigger changes leave that flag clear. The unsuccessful periodic refresh was removed. Tests verify those report flags on USB and Bluetooth. **Side isolation was unresolved at this stage; the video/revision follow-up below supersedes that conclusion for the attached device.** No firmware update/reset or permanent Steam configuration changes were attempted.

`ats-dualsense/scripts/diagnose-player-leds.py` preserves the explicit parked/paused isolation test for reproducibility. It checks the Steam PID and Sony USB HID identity, requires `--game-paused`, sets an independent resume timer, and restores Steam and the prior service state in cleanup. It is an interactive diagnostic, not an automated qualification test. Run it only while parked and paused.


## Video and hardware revision follow-up — October 5, 2026

The user supplied a private 3.375-second, 24 fps controller video. Frame review shows both inner player LEDs at 0.042–0.125 seconds, all four outer LEDs at 0.167–0.375 seconds, then off; subsequent cycles repeat this symmetric sequence. The center is off. The RGB side strips remain blue and are distinct from the blinking white indicators. The video is retained locally and is not published.

A read-only firmware feature report (`0x20`, 64 bytes) on the same USB controller returns hardware info `0x00000514` at bytes 24–27: generation 5, trial 0x14. This supersedes the earlier unknown-cause conclusion for the attached device: its physical behavior is consistent with mirrored inner/outer LED pairs. It is a revision capability limitation, not evidence that ATS requests both turns.

Primary implementation sources document generation 4 mirroring: [SpecialK output layout](https://github.com/SpecialKO/XInput_HID/blob/master/dualsense.cpp) and [DualSense Client lighting controls](https://dualsenseclient.github.io/DualSenseClient/guides/light-control/). Generation 5 is classified from the attached device's report plus the user video, not from a claim that these sources qualify all generation-5 devices. [Linux hid-playstation](https://github.com/torvalds/linux/blob/master/drivers/hid/hid-playstation.c) supplies the firmware-report hardware offset and transport layout.

At the user's request, known standard generation-4/5 controllers now use HUD-only individual turns, keeping white LEDs for truck lights and hazards. Generations 2/3 retain directional requests. Unknown generations and Edge remain explicitly unqualified. Snapshot/diagnostics expose hardware info and LED layout; cockpit/HUD arrows use logical turn channels, not the player mask. Automated regressions cover revision classification, lights/turns/hazards on mirrored hardware and logical HUD isolation even when both physical lamp channels are true. After installing the tested daemon and optional desktop HUD, the user confirmed that the selected fallback works: individual turns remain in the HUD, the unwanted bilateral white-LED sweep is absent for single turns, and hazards retain symmetric LED output. This qualifies the selected fallback on the attached generation-5 USB controller; it does not establish independent side control. The broader driving/USB/Bluetooth/Edge qualification gates remain open.


## 2026-10-06 web navigator

- Native Release build, CTest (3/3), undefined-behavior-sanitized CTest (3/3), HTTP/UDP integration, dashboard/HUD/navigation DOM regressions and map-converter fixtures passed. Integration covers exact v4/v5/v6 migration and rejection of nonfinite placement. The freestanding x64 Windows DLL built with locally extracted Clang/LLD 18; no system toolchain installation was required.
- Chromium review passed desktop (1600 px) and mobile (390 px), no horizontal overflow or page errors, camera/orientation/zoom controls, import/unload, retained valid maps after invalid import, error persistence across polling, fullscreen and stale-position clearing. Rendered screenshots are generated under `build/web-review-*.png`.
- Read-only extraction from installed ATS **1.61.3.1** completed with TruckSim Maps at `d56d0e3` in a temporary checkout. An extractor-only workaround removed its speed-limit-array length assertion for this game version; speed-limit definitions are not used by the HaulSense road export or cockpit limit. The extractor warned about unrecognized `dlc_sd.scs`, omitted several city-area associations and unrelated achievement definitions; full DLC coverage is not claimed. No game data or saves were changed by extraction.
- The local, untracked `build/ats-roads.json` contains **46,511 merged road centerlines, 226,497 points and 289 city labels**, about 4.6 MiB. Degree-two joins and one-metre simplification preserve road connectivity without filling missing prefab intersections. No missing-node roads were skipped by the converter. The full map loaded in Chromium in about 121 ms; ten local draw calls averaged about 0.87 ms (maximum about 1.1 ms). These are one-machine observations, not a general performance guarantee.
- The real road layer was visually reviewed around Bakersfield with a **simulated** truck position, explicitly labelled DEMO. This qualifies dataset import/rendering, not live game position alignment. The view is 2.5D perspective; prefab intersections, 3D buildings/terrain, traffic and planned-route geometry remain absent.
- Installed native binary and served navigation asset hashes match the tested build. The configured user service was restarted and is active. The v6 DLL was installed atomically; previous binary/DLL/config copies are in `build/pre-web-install-20261006-223126/`. Existing config, Steam Input and HUD settings were preserved. ATS must restart to load the new DLL; a live v6 driving session and actual map alignment remain unqualified.

## Detailed scene and future route — 2026-10-07

This supersedes the earlier centerline-only navigator limitations; those dated results remain historical evidence.

- Release CTest and UBSan CTest passed **4/4**, including the new asynchronous route reader. Fixtures verify content-change freshness/expiry, inactive telemetry suppression, nonfinite provider rejection and a FIFO that cannot block shutdown. HTTP integration verifies embedded scene/worker assets and `/api/route` alongside existing v4/v5/v6 lifecycle/settings checks. Dashboard/HUD/navigation DOM, rich map validation, exporter transforms/polygons/sign text and directed router fixtures passed. Remote CI was not executed.
- `build/ats-scene.json` contains **47,383 roads, 289 cities, 50,551 visible prefabs, 235,305 measured object bounds and 93,891 placed signs**, approximately 82 MiB. Its graph has **193,524 nodes / 168,687 road edges**, with prefab connections expanded in the worker. The converter reports no missing optional files, prefab descriptions or model bounds for this parser output; that does not qualify every DLC/mod. Historical parser warnings still apply.
- A real extracted Bakersfield–Los Angeles graph route resolved through directed roads and prefab curves. Another sampled run visited 1,668 nodes and returned 1,167 route points in approximately 5.4 ms. Distance is in map metres, not the scaled game navigation distance. This is an independent path with no guarantee of matching game GPS preferences.
- Chromium WebGL review passed: actual extracted 3D scene, road curves/elevation, instanced bounds, sign text, POIs, layer/camera controls, 2D fallback, fullscreen and 390-pixel mobile layout without horizontal overflow or JavaScript errors. Screenshots `build/web-review-scene{3d,2d,-fullscreen,-mobile}.png` use **DEMO simulated placement**. Original meshes/textures and terrain are absent; traffic and semaphore phases are unknown.
- Browser tests with an explicitly simulated provider verified Game GPS source selection, fallback after staleness, UID mismatch rejection, pause/game mismatch clearing and loss-of-WebGL-context fallback. `tests/browser-scene.cjs` makes these checks reproducible with the local detailed map and an isolated mock daemon. The actual ETS2LA file/provider was not present on this host; real provider operation under Proton remains unqualified.
- A local Chromium sample after batching recorded **40 draw calls, 16,618 triangles, 8 geometries and 20 textures**. Ten cached draws took 1.1–1.8 ms of JavaScript submission time, averaging about 1.46 ms. This excludes GPU completion and static geometry rebuild cost and is not a general FPS/memory guarantee. Metrics: `build/scene-render-metrics.json`.
- Live v6 game-driving/map alignment, road width accuracy, original scenery reconstruction, complete DLC/mod coverage, ferry/restriction routing and a long-haul performance run remain open. Existing physical controller qualification is separate.

- Installed daemon and all served JavaScript assets match the tested build; `haulsense.service` is active. Prior executable/DLL and unchanged config/HUD preferences were preserved in `build/pre-scene-install-20261007-070558/`. This scene update did not replace the already installed v6 DLL or restart ATS. The provider endpoint reports unavailable on the actual installed service.
