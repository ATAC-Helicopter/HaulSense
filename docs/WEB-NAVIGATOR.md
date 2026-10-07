# Web navigator

The navigator follows SDK world placement in the X/Z horizontal plane (north is -Z). Heading is a counterclockwise turn fraction: 0 north, .25 west, .5 south, .75 east. Pitch/roll also use turns. Position is double precision on the wire. V6 adds placement; v5/v4 still work with position unavailable. Restart ATS after replacing the DLL.

## Load a local scene

Use **Load road map** in the cockpit and select a game-matching JSON. On this workstation, `build/ats-scene.json` contains the installed ATS 1.61.3.1 map: 47,383 road lines, 289 cities, 50,551 visible prefab instances, 235,305 object instances and 93,891 placed signs. The complete detailed file is approximately 82 MiB. Older v1 centerline maps remain supported.

**3D / Perspective** uses an embedded, offline Three.js WebGL renderer. Without WebGL or with a v1 map, a canvas perspective/2D fallback remains available. The default **Chase camera** follows an original generic tractor, with a straight trailer proxy only when SDK coupling is active. **Overview camera** restores the wider navigator; north-up selects overview. Camera preference survives reload. Controls also select zoom, fullscreen and props/signs/details. Mobile places the navigator above instruments. Pause, missing placement and game mismatch hide the scene. Nothing is uploaded. Imported maps persist in IndexedDB and restore after refresh/relaunch; Forget map removes the cached copy. The standalone can bootstrap a local installed `maps/ats.json`. Session-only cyan history remains distinct from the native persisted job route.

The scene includes road elevation, extracted lane counts/shoulders, prefab road surfaces and navigation curves, roadside signs with extracted text, semaphore positions, dividers and POIs. Original procedural building/house roofs, vegetation, lamps, bollards and generic prop shapes use measured bounds and map-provided placements. Very large panorama bounds remain low footprint markers to avoid opaque blocks enclosing the street. The current local export provides building model descriptions; vegetation and other model classes render only when included in an imported map. No decorative positions are invented. Road widths use a 3.5-metre lane estimate plus extracted offsets/shoulders, with geometric edge/centre paint and dashed lane dividers. These are schematic marking rules, not decoded road material meshes. Signs use a readable schematic style; atlas names can appear as text. Original game meshes/textures, terrain surfaces and animated traffic are absent. Only live semaphore phases require the optional ETS2LA provider; missing/stale input leaves them unknown. Static signals use dark three-lamp housings; live heads show the provider position/state with a readable camera-facing symbolic orientation. Divider rails are symbolic representations of extracted divider paths. This is a schematic navigator, not a complete replica of the rendered game world.

Objects are indexed in 500-metre buckets; roads use bounding boxes. Visible radius is capped at 1,400 metres; object instances at 2,200, placed signs at 180 and nearby sign labels at 24. Prefab templates avoid duplicating intersection geometry in the JSON. Road triangles/curve segments are batched and objects instanced. A floating local origin preserves GPU precision. Static geometry rebuilds on 80-metre camera cells, zoom/layer changes or new data; static scene/route input updates at dashboard cadence while camera/model interpolation renders at most 30 FPS. Hidden views and paused/missing placement stop the animation loop; resuming restarts it. Shortest-angle interpolation handles the heading wrap and large teleports snap immediately. WebGL pixel ratio is capped at 1.5. Repeated overlays share signal primitives; visible sign-text canvases are recycled. Dash generation and divider posts are bounded. Tractor pitch/roll follow telemetry, while generic dimensions, mapped road contact (within 10 metres horizontally and five metres vertically, otherwise a generic chassis offset), straight trailer shape and symbolic lamp facing are approximations. No trailer world pose or exact truck model mesh is claimed.

## Future route

Orange indicates the future route; cyan is the measured travelled trace (at most 1,200 samples).

**HaulSense route** uses a Web Worker and an A* graph with extracted road directions and prefab navigation connections. Choose a city or use automatic job destination matching. It reroutes after movement and never writes to the game's GPS. It is a mapped, driveable geometric path, not the game's chosen route. It does not account for truck orientation, temporary closures, ferry services, truck restrictions or the game's routing preferences. City endpoints snap to mapped nodes; offsets are not rendered as invented road connections. Disconnected paths return an error.

**Game GPS route · ETS2LA provider** is optional. The daemon reads `/dev/shm/ETS2LARoute` (override with `--route-file PATH`) using the [ETS2LA NavigationProvider ABI](https://github.com/ETS2LA/ETS2LA/blob/main/ETS2LA.Game/SDK/Navigation.cs): 6,000 little-endian records of a 64-bit node UID and two float32 distance/time values, 96,000 bytes. The optional installer can install a pinned upstream 1.61 provider; HaulSense itself does not read game process memory. The pinned provider detects Wine/Proton and creates Linux-accessible shared files. A Windows named mapping alone is insufficient.

The file is read on a separate native thread, read-only, with owner/type/size checks and two matching reads. Its ABI has no heartbeat: freshness requires an observed content change, expires after 2.5 seconds, and additionally requires live non-demo telemetry. A stationary, unchanged route can therefore be conservatively withheld. Browser polling is at most once per second while active, with automatic destination selected. Every UID and directed connection must match the imported map; otherwise the route is rejected. No straight-line links are invented. HaulSense routing remains available when the provider is absent/stale. Exact-provider integration has fixture coverage; live ATS/Proton provider operation remains unqualified.

The SCS SDK alone supplies navigation distance, time and speed limit, not future route geometry. A manually selected GPS destination has no city name in these channels. The city selector remains available for independent navigation.

## Export from your game

Use the [TruckSim Maps parser](https://github.com/truckermudgeon/maps) to extract installed ATS/ETS2 archives into a directory, then run:

```bash
python3 ats-dualsense/scripts/export-road-map.py /path/to/parser-output /path/to/scene.json --game ats
# Optional smaller region in SCS metres:
python3 ats-dualsense/scripts/export-road-map.py /path/to/parser-output /path/to/region.json --game ats --bounds -106000 5000 -100000 18000
```

The converter reads `usa-*` / `europe-*` arrays: nodes, roads, roadLooks, cities, prefabDescriptions/prefabs, modelDescriptions/models, signDescriptions/signs, dividers and pois. Missing optional datasets are recorded in `coverage`; roads with missing nodes are counted. Hidden/secret roads and prefabs are excluded. Preserve 64-bit UIDs as hexadecimal strings. Elevation comes from the parser's node `z`; its `y` is game world Z. No proprietary game assets or map files are shipped in the repository.

The local source is retained in `build/map-source/` and `build/map-extraction.log`. Parser revision `d56d0e3` needed a temporary speed-limit-array assertion workaround for ATS 1.61; speed-limit definitions are not used by this scene. The parser reported an unknown South Dakota archive and other warnings; this is not proof of complete DLC coverage. Re-extract after game/mod changes. ETS2 extraction/alignment has not been qualified on this workstation.

## Format and validation

Both versions use `game: "ats" | "ets2"`, `coordinates: "scs-xz-metres"`, `roads` and optional city `labels`. Road points are `[x,z]` or `[x,z,elevation]`. V2 adds roadStyles, reusable prefab templates/placements, asset bounds/objects, sign definitions/placements, barriers, POIs and a directed graph. Graph road edges may include their sampled curved geometry. See the converter for array schemas.

Import limits: 96 MiB, 150,000 road lines, 1,500,000 road points and 2,000 labels. Scene arrays and references have individual bounds; coordinates must be finite and below 100,000,000 in magnitude. Labels are text-only. Invalid imports preserve the previous valid map. Geographic GeoJSON is not accepted without a correct game projection.

## Journey and validation

Fuel margin is estimated range minus the remaining game GPS distance; it is not computed from the independent route. Odometer delta, cargo condition, cab attitude and telemetry warnings use measured inputs and clear on pause/disconnect. Navigation time is game travel time, not a delivery deadline.

Run CTest, `tests/integration.py`, `tests/map_export.py`, `tests/routing.cjs` and the navigation/dashboard/HUD DOM regressions. Browser screenshots in `build/web-review-scene*.png` show the actual extracted map with a clearly labelled **simulated** truck position. They qualify rendering and controls, not live driving alignment. See [qualification](QUALIFICATION.md).

The minified Three.js bundle is checked in, so CMake and runtime require no npm/network access. After editing `ui/scene.js`, rebuild it with `ats-dualsense/scripts/build-scene.sh` (npm, pinned Three.js 0.180.0 and esbuild 0.25.6). The MIT license is retained under `third_party/three/`.

Optional browser lifecycle regression (requires the local detailed map, Playwright and an isolated `--mock --no-controller` daemon at port 39076):

```bash
NODE_PATH=/path/to/playwright/node_modules node tests/browser-scene.cjs build/ats-scene.json http://127.0.0.1:39076
```

Set `PLAYWRIGHT_CHROMIUM_EXECUTABLE_PATH` if using an existing Chromium executable instead of Playwright's installed browser.

Original texture-free fixture regression (isolated mock daemon, no private game map required): `node tests/scene-geometry.mjs` and `NODE_PATH=/path/to/playwright/node_modules node tests/browser-procedural.cjs http://127.0.0.1:39076`.
