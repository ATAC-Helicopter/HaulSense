# Changelog

## Unreleased — compact HUD and controller diagnostics

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
