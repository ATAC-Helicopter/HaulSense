# Live signal and game-route provider

SCS Telemetry SDK 1.14 does not expose traffic-light phases. The separate ETS2LA plugin reads the engine's semaphore objects and publishes them. This is a real data source; static map semaphore profiles alone cannot synchronize a timer to the game.

Primary references: [plugin semaphore writer](https://github.com/dariowouters/ets2la_plugin/blob/c925bd965a96c58b73eaea9ca033fe0f0dc623a7/src/processing/traffic.cpp), [packed fields](https://github.com/dariowouters/ets2la_plugin/blob/c925bd965a96c58b73eaea9ca033fe0f0dc623a7/src/core.hpp), [ETS2LA reader](https://github.com/ETS2LA/ETS2LA/blob/v2026.10.5115/ETS2LA.Game/SDK/Semaphores.cs).

## Installed provider

The optional installer downloads the **1.61 Windows DLL**, from ETS2LA release `v2026.10.5115`, commit `41945bc41e36191fbdf277c7fa6f7dd6f101d679`, SHA-256 `0e1893719f84f28079857451b82a22a263401d98b8fc299cd561eb7695bfa88f`. It verifies the checksum before atomic installation and backs up an existing provider. The separate upstream plugin is MIT licensed; it is not vendored into HaulSense source or enabled for other game versions. Restart ATS to load it. HaulSense never enables its steering/acceleration overrides.

The provider detects Wine/Proton and explicitly creates Linux-accessible files under `/dev/shm`. A Windows named mapping without that bridge is insufficient. The previous navigator documentation's assumption that a separate Linux writer was always required is superseded by this verified upstream capability.

## Semaphore ABI

`/dev/shm/ETS2LASemaphore`: 40 records × 48 bytes = 1,920 bytes. Little endian, packed:

| Offset | Type | Meaning |
| --- | --- | --- |
| 0/4/8 | float32 | local x/y/z |
| 12/14 | int16 | world cell x/z |
| 16/20/24/28 | float32 | quaternion w/x/y/z |
| 32 | int32 | type: 1 light, 2 gate, 0 unused |
| 36 | float32 | remaining time, seconds |
| 40 | int32 | state |
| 44 | int32 | semaphore ID |

World position is `[x + cellX×512, y, z + cellZ×512]`. Light states: 0 off, 1 amber towards red, 2 red, 4 amber towards green, 8 green, 32 flashing amber. Gate states are separately interpreted; no light state is assigned to gates. A flashing illustration is not a measurement of the engine's exact blink phase.

A read-only native thread samples at 10 Hz, validates file owner/type/exact size, compares two reads and checks states/coordinates/time. The ABI has no writer heartbeat; content must change before being called fresh and expires after 1.5 seconds. Missing, corrupt, unchanged/stale or paused data produces an empty live list. `/api/signals` reports source, availability, freshness and objects. Frontend colours are cleared on expiry and never assigned from static profiles. The nearest light's state/time is displayed without claiming it applies to the truck's lane/approach.

`--semaphore-file PATH` selects an alternate fixture/provider file. `--route-file PATH` overrides the separate 96,000-byte route ABI; see [navigator](WEB-NAVIGATOR.md). Both feeds additionally require active non-demo telemetry.

The pinned provider and file ABI were verified from first-party source/binary; native fixtures and rendered synthetic provider tests are separate from a real driving session. A successful DLL installation is not proof of live compatibility with every ATS 1.61 patch/mod or TruckersMP. Verify actual `/api/signals` freshness after game restart before treating live integration as qualified.

## Standalone integration

The desktop bundle carries the pinned 1.61.x provider and its MIT license. It discovers Steam installations and installs the DLL automatically only after its embedded map verifies the game version/content and names a supported 1.61.x version. Existing different DLLs are backed up; a new installation requires restarting the game. Waiting for telemetry is displayed separately from provider installation. Packaging downloads/checks the immutable DLL at build time; the renderer never downloads code. The standalone does not claim that an installed DLL alone proves a fresh live signal/GPS feed.
