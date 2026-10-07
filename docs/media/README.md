# HUD review images

Captured locally on 2026-10-05 from the actual HUD implementations:

- `hud-gps.png`: GTK HUD, 340×273, telemetry fixture with a GPS route and no job, metric units.
- `hud-web-demo.png`: browser HUD, 400×500 viewport, hardware-free demo fixture, rendered in isolated headless Firefox.

These are rendered software views, not in-game capture or evidence of physical DualSense LED correctness. Controller side isolation remains unresolved. The FG Labs product page reuses these exact assets with visible fixture/demo captions.

## Standalone candidate (2026-10-07)

- `standalone-desktop-demo.png`: actual Electron cockpit with hardware-free DEMO telemetry and a locally imported schematic ATS road scene. Props are bounds; this is not original game rendering.
- `standalone-job-report-demo.png`: actual rendered journal view with explicitly synthetic delivery values and a sampled mapped route; it is not a driven delivery. The native save-dialog export was also verified.

The images prove rendered layout only. Live provider transitions, sustained driving performance and v7 delivery remain qualification gates.
