# Changelog

## Unreleased — original low-poly driving scene

- Replaced the triangle position marker with an original generic tractor and optional straight trailer proxy, following measured world pose.
- Added a smoothed 30 FPS chase camera, persistent overview switch and shortest-angle heading interpolation; hidden/paused views stop animation.
- Added joined elevated road ribbons, asymmetric carriageways, shoulders, edge/centre lines and dashed lane dividers using simple geometry. Intersection navigation curves remain routing data, not invented lane paint.
- Added original procedural house roofs, vegetation/lamp/bollard proxies where the map supplies them, physical sign boards, mapped divider rails and three-lamp semaphore housings. Fresh provider colours remain distinct from unknown static lights.
- Uses solid flat colours and instanced primitives; only small local sign-text canvases use textures. No ETS2LA renderer code, original game meshes/textures, invented prop placements or real trailer articulation are included.
- Added geometry/input bounds, camera/visibility/lamp/memory render regressions and labelled synthetic scene screenshots. Real driving alignment and sustained performance still require qualification.

## 1.0.0-rc.1 — standalone navigator and job journal

- Added a sandboxed Electron desktop with persistent local origin/map cache, native menu/export, bounded background updates and revised route-first cockpit.
- Added native per-frame job recording, resumable checkpoints, travelled route/report history and official v7 delivery/cost metadata. V4/v5/v6 prefixes remain compatible.
- Added actual semaphore-state reader and optional checksum-pinned ETS2LA 1.61 provider installer; missing/stale input clears phases.
- Removed executable inline scripts, hardened HTTP headers/Origin/CSP and desktop permissions/navigation/fuses. Added weekly dependency tracking and CodeQL/dependency review.
- Candidate status keeps live game/provider/long-haul and broader device qualification explicit.

### Included web scene, compact HUD and controller diagnostics

- Added v6 double-precision world placement and game identity while preserving exact v5/v4 compatibility.
- Added a local WebGL schematic navigator with extracted road elevation, prefab surfaces/curves, signs, semaphore positions, dividers, POIs and instanced measured object bounds; retained 2D/perspective fallback and fullscreen/mobile controls.
- Added offline directed A* city/job routing through prefab connections and an optional read-only ETS2LA UID-route reader with freshness, map validation and explicit source labels.
- Added the detailed scene converter, bounded spatial indexing/batched rendering and measured journey/fuel/cargo advice. Original game meshes/textures, terrain and live traffic remain absent.
- Added route-reader, routing, rich scene/export and browser lifecycle regressions. Extracted ATS rendering is locally verified with simulated placement; live alignment/provider operation remains unqualified.


- Added embedded browser HUD and optional transparent GTK desktop overlay with persistent monitor, position, units, opacity and size preferences.
- Centered speed, limit and trip information; show GPS distance/ETA without a job and distinguish job destination metadata.
- Added gear, fuel range, cruise target and contextual warnings; native HTTP waits run outside the GTK main thread.
- Replaced sequential sysfs gameplay writes with atomic instant HID reports; restrict direction masks and suppress the center during signals.
- Send player LED commands only on mask changes, independently of RGB, trigger and rumble updates.
- Added HUD DOM, USB/Bluetooth report and job-metadata regressions, plus a bounded interactive controller isolation diagnostic.
- Read the controller hardware revision: standard DualSense generations 4/5 use mirrored player LED pairs. User video and hardware `0x00000514` confirm the attached generation-5 behavior.
- On mirrored revisions, show individual turns in the HUD only; retain white LEDs for truck lights and hazards. Unknown/Edge revisions are reported as unqualified.
- Drive HUD/cockpit arrows from logical turn requests and the shared lamp clock, independently of physical LED masks. No stable hardware qualification is claimed.

## 0.8.0 alpha — HaulSense

- Named project, public governance, continuous validation and FG Labs product entry.
- Embedded local cockpit, metric/US units, bounded history, SDK inspector and persistent fine tuning.
- Shared protocol and official SDK 1.14 headers; explicit channel availability, gameplay events and primary-trailer data.
- Separate left/right LED sweeps, hardware side calibration and controller-associated sysfs discovery.
- Correct kernel bus transport detection; changed-LED writes and cached brightness.
- 50 Hz send/output ceilings, bounded backlog draining, exact packet validation and immediate SDK pause neutralization.
- Fixed trigger zero-disable and terrain feedback caused by persistent suspension spread.
- Tuned short-event/impact gains; optional reverse/wiper rhythms, trailer/lift-axle/gameplay acknowledgements.
- Exact legacy packet adapter and atomic DLL replacement for uninterrupted upgrade sessions.
- Safe hardware-free demo, legacy config/executable migration, desktop launcher and clean uninstall.

## 0.7.1 — ATS DualSense Bridge

Original imported source, preserved in the first repository commit and a local archive. Legacy directional LED and quiet-immersion foundation.
