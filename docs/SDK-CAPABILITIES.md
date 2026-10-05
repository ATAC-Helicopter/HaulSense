# SCS SDK: useful data and HUD semantics

The official vendored SDK 1.14 headers are the contract. Channel availability remains explicit: absent values are `null`, not zero. HUD features reuse the existing packet, so the running v5 game plugin remains compatible.

| SDK data | Use in HaulSense |
| --- | --- |
| Speed, advisor speed limit | Centered speed and limit, overspeed colour; the game can disable the advisor limit |
| Navigation distance and ETA | Remaining travel even with no active job; metres and game seconds converted for display |
| Job configuration destination city | Localized delivery city, cleared when the job configuration becomes empty |
| Displayed gear, fuel range, cruise target | Compact driving information; range is in kilometres, cruise target in m/s |
| Fuel, air, oil, water, battery warnings, parking brake | Contextual HUD warning row, hidden when there are no warnings |
| Logical signals and physical lamp channels | Logical channels select the controller side; lamp state clocks the sweep and HUD arrows |
| Suspension, wheel ground/velocity, braking, engine and trailer states | Existing short controller feedback cues and dashboard inspector |
| Gameplay events | Existing delivery/fine/toll acknowledgements |

A manually selected GPS waypoint does **not** expose its city/name through these SDK channels. `job.destination.city` is not a channel: the name comes from the configuration event's `destination.city` attribute for an active job. A route without that name is displayed as “Percorso GPS”, with distance and ETA still visible. Zero navigation distance/time and no job is “Nessun percorso”. Paused or expired telemetry clears the HUD.

Useful further extensions supported by the SDK, not yet added to the wire protocol:

- Job company names, cargo loading state and planned distance for a richer delivery card.
- Job income and delivery window plus the global game-time channel for an actual deadline countdown. Navigation ETA is travel time, not the delivery deadline.
- Trailer identity and indexed configurations for multi-trailer diagnostics.
- Gameplay event attributes for the actual fine/toll/delivery amount, beyond the current event ID acknowledgement.

These require versioned packet changes and migration tests. The current SDK does not provide turn-by-turn GPS instructions or an ABS activation flag; do not synthesize those as measured facts. Wiper feedback remains a synthetic rhythm from an on/off state.

References: [official telemetry overview](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry), vendored `scssdk_telemetry_common_configs.h`, `scssdk_telemetry_truck_common_channels.h`, `scssdk_telemetry_common_channels.h` and `scssdk_telemetry_event.h`.
