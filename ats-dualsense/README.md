# ATS DualSense Bridge v0.7.1

Native Linux DualSense effects for the Windows/Proton build of American Truck Simulator.

`ATS.exe -> SCS telemetry DLL -> localhost UDP -> native Linux daemon -> DualSense HID + hid-playstation player LEDs`

## v0.7.1 hotfix

- Fixed Linux player-LED discovery for the actual `hid-playstation` names (`player-1` through `player-5`).
- Player LED discovery also accepts the legacy no-hyphen spelling for compatibility.
- DualSense hidraw enumeration is now sorted numerically so the service and diagnostics consistently select the same controller interface.

## v0.7.1 highlights

- **Directional indicators fixed at the state-engine level.** ATS logical `left_blinker` / `right_blinker` channels choose the side; physical blinker-lamp telemetry is used only as the animation clock. A left signal can no longer light the right pair merely because both physical lamp channels pulse.
- **Sequential white LEDs:** inner -> inner+outer on the selected side. The center white LED independently mirrors truck exterior lights.
- **Live player LEDs are sysfs-only.** Raw HID player masks are deliberately disabled during gameplay because some DualSense firmware/kernel combinations render those masks as non-directional patterns.
- **Red-flash cleanup.** Normal component wear no longer causes red flashing. Wear warning thresholds were raised to 65% / 85%, and oil/battery warnings are gated by engine state. Fuel and air warnings use sparse amber cues; red is reserved for real critical states.
- **Brake feel v2:** progressive L2 resistance plus a restrained ~10 Hz brake texture only under genuinely heavy braking while moving. Brake onset also gets a short tactile pulse.
- **Slightly stronger event feedback** without restoring continuous throttle/RPM rumble. Road joints, gear changes, engine-brake engagement, retarder changes and engine start are short events.
- **Calmer lightbar:** stable dark blue/light-state base, white reverse, amber hazards, slow gold beacon, brief cruise/diff/high-beam/parking-brake acknowledgements.
- **Hot-plug/reconnect**, fast ATS pause/exit neutralization, diagnostics, version output, and signal tracing.
- **One-command install:** `./scripts/install-all.sh`.
- **Clean uninstall:** `./scripts/uninstall.sh` keeps the user's config and udev rule.

## Install / update

Close ATS first, extract the release, then:

```bash
cd ~/Downloads/ats-dualsense
./scripts/install-all.sh
```

The installer updates the native daemon, udev access, Windows x64 SCS telemetry DLL, and restarts/enables the user service.

If automatic ATS discovery is ever needed elsewhere:

```bash
ATS_DIR='/path/to/American Truck Simulator' ./scripts/install-all.sh
```

## Diagnostics

```bash
ats-dualsense --version
ats-dualsense --diagnostics
```

For the five white LEDs, stop the service before direct testing:

```bash
systemctl --user stop ats-dualsense.service
ats-dualsense --led-test
systemctl --user start ats-dualsense.service
```

To inspect exactly what ATS is reporting for left/right indicators:

```bash
systemctl --user stop ats-dualsense.service
ats-dualsense --telemetry-debug
```

Then launch ATS. Signal changes print as:

```text
signals L=1 R=0 lampL=1 lampR=1 hazards=0 ledMask=0x8
```

Even if both physical lamp channels happen to pulse, `L=1 R=0` means only the left player-LED pair is selected in v0.7.1.

## Player LED layout

Physical layout is treated as:

```text
left                                      right
player5   player4   player3   player2   player1
   ●         ●         ●         ●         ●
```

- Left: `player4` -> `player5 + player4` -> off.
- Right: `player2` -> `player2 + player1` -> off.
- Center `player3`: parking/low/high beam state.
- Hazards: both sequences plus amber RGB lightbar.

## Default config

`~/.config/ats-dualsense/config.conf`

```ini
rumble_strength=0.22
road_strength=0.28
trigger_strength=0.66
brake_haptic_strength=0.38
lightbar_strength=0.68
beacon_strength=0.24
bump_threshold=0.46
overspeed_warning=false
critical_lightbar_warnings=true
sysfs_player_leds=true
```

The actual key is `overspeed_warning` (shown correctly in the packaged config). Existing customized configs are preserved; untouched v0.6 defaults are backed up and migrated automatically.

## Effect model

### Lightbar

- Electrics/engine: restrained blue base.
- Parking/low/high beam: progressively brighter/cooler blue.
- Reverse: white.
- Hazards: amber, synchronized to ATS's physical blinker phase.
- Beacon: slow, dim gold breathing.
- Fuel/air warnings: sparse amber.
- Oil/water, air emergency and critical wear: sparse red only when appropriate.

Normal left/right indicators never recolor the RGB lightbar.

### Adaptive triggers

- **L2:** progressive brake resistance beginning at light pedal input, with stronger feel toward full braking. Low-air states increase resistance.
- **R2:** normally free; only a tiny end-of-travel cue near full throttle/high engine demand.

### Rumble / brake tactile layer

There is no continuous RPM/throttle rumble. Feedback is event/transient driven:

- gear change
- road/suspension impact
- engine start
- engine-brake engagement
- retarder level change
- hard brake onset
- restrained brake texture during genuinely heavy braking
- air-pressure / critical warnings

This is compatible rumble plus adaptive-trigger feedback. Sony-style USB audio haptics are a separate path and are not claimed as implemented here.

## Logs

```bash
journalctl --user -u ats-dualsense.service -f
```

Expected connection message:

```text
ATS telemetry connected. Immersion engine v0.7.1 active.
```

## Uninstall

```bash
./scripts/uninstall.sh
```

This removes the daemon, user service and ATS plugin, while preserving `~/.config/ats-dualsense/config.conf` and the udev permission rule.
