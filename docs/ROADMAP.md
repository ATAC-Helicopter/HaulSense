# Roadmap

## Foundation (0.8 alpha)

- Live local cockpit, tuning presets and SDK channel inspector.
- Shared official SDK adapter and validated wire protocol.
- Independent animated directional LEDs and calibrated side swap.
- Bounded CPU/network cadence, hidden-tab suspension, changed-output writes.
- Gameplay, trailer coupling and lift-axle cues with opt-in reverse/wiper rhythms.

## Web navigator

- Implemented: v6 placement with v5/v4 compatibility; local WebGL schematic scene and 2D fallback; extracted road elevation, prefab surfaces/curves, object bounds, signs, dividers and POIs; spatial index, batching/instancing and layer controls.
- Implemented: directed A* routing through roads and prefabs, city/job destination selection, and optional read-only ETS2LA node-route input with freshness and map-connection validation.
- Locally checked: ATS 1.61 extracted map rendering, Bakersfield–Los Angeles routing, desktop/mobile/fullscreen controls and provider fixtures.
- Open: original meshes/textures and terrain, comprehensive game/DLC/mod coverage, route restrictions/ferries/preferences, live ATS/ETS2 alignment and actual ETS2LA provider operation under Proton. Do not equate bounding-volume scenery with the full game environment.

## Qualification before stable

- Rendered desktop/mobile review of the dashboard, keyboard and screen-reader review.
- Recorded ATS driving session covering left/right/hazards, pause/resume, truck switches, job delivery, road tuning and config persistence.
- USB/Bluetooth and DualSense Edge reconnection/neutralization evidence across firmware/kernel versions.
- Game-version channel availability audit and long-haul performance samples.

## Later, only where the SDK supplies evidence

- ETS2 and native Linux telemetry plugins, with separate installation and compatibility tests.
- Multi-trailer inspection, calibrated axle positions and optional per-surface road profiles.
- Device selection for multiple controllers, battery telemetry and per-device calibration.
- Optional true USB audio haptics, with isolation from the game's audio routing.

Do not label synthetic wiper timing as measured motion, brake texture as ABS detection, or compatible rumble as audio haptics. Driving qualification is a separate gate from fixture/CI success.

## 1.0.0-rc.1

- Standalone isolated desktop, persistent maps/preferences, background focus behaviour and revised cockpit/job log.
- Native asynchronous job journal and v7 typed delivery/cost attributes with exact legacy migration.
- Actual signal-state reader and checksum-pinned optional ATS/ETS2 1.61 provider installation.
- Strict script CSP/HTTP parsing, locked desktop dependencies/fuses, CodeQL/dependency review and repository secret/push protection.
- Stable gate: live v7 delivery/cancellation and actual signals/GPS under Proton after restart, long-haul performance/focus, broader transports/revisions and game/DLC coverage. A candidate build is not completion of these gates.
