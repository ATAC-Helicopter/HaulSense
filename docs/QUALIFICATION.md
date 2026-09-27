# Qualification evidence — 0.8 public alpha

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
