# SCS SDK: useful data and HUD semantics

The official vendored SDK 1.14 headers are the contract. Channel availability remains explicit: absent values are `null`, not zero. HUD features reuse existing channels. The navigator adds a v6 placement suffix; running v5 game plugins remain compatible with the daemon but cannot provide map position.

| SDK data | Use in HaulSense |
| --- | --- |
| Speed, advisor speed limit | Centered speed and limit, overspeed colour; the game can disable the advisor limit |
| Navigation distance and ETA | Remaining travel even with no active job; metres and game seconds converted for display |
| Job configuration destination city | Localized delivery city, cleared when the job configuration becomes empty |
| Displayed gear, fuel range, cruise target | Compact driving information; range is in kilometres, cruise target in m/s |
| Fuel, air, oil, water, battery warnings, parking brake | Contextual HUD warning row, hidden when there are no warnings |
| Logical signals and physical lamp channels | Logical channels select the controller side; lamp state clocks the sweep and HUD arrows |
| Suspension, wheel ground/velocity, braking, engine and trailer states | Existing short controller feedback cues and dashboard inspector |
| World placement (dplacement) | v6: double-precision x/y/z, heading/pitch/roll and game identity for the local navigator; no-value callbacks clear availability |
| Odometer, fuel range, cargo damage | Journey distance, estimated range margin and cargo condition |
| Gameplay events | Existing delivery/fine/toll acknowledgements |

A manually selected GPS waypoint does **not** expose its city/name through these SDK channels. `job.destination.city` is not a channel: the name comes from the configuration event's `destination.city` attribute for an active job. A route without that name is displayed as “Percorso GPS”, with distance and ETA still visible. Zero navigation distance/time and no job is “Nessun percorso”. Paused or expired telemetry clears the HUD.

Useful further extensions supported by the SDK, not yet added to the wire protocol:

- Job company names, cargo loading state and planned distance for a richer delivery card.
- Job income and delivery window plus the global game-time channel for an actual deadline countdown. Navigation ETA is travel time, not the delivery deadline.
- Trailer identity and indexed configurations for multi-trailer diagnostics.
- Gameplay event attributes for the actual fine/toll/delivery amount, beyond the current event ID acknowledgement.

These require versioned packet changes and migration tests. The current SDK does not provide turn-by-turn GPS instructions or an ABS activation flag; do not synthesize those as measured facts. Wiper feedback remains a synthetic rhythm from an on/off state.

References: [official telemetry overview](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry), vendored `scssdk_telemetry_common_configs.h`, `scssdk_telemetry_truck_common_channels.h`, `scssdk_telemetry_common_channels.h` and `scssdk_telemetry_event.h`.

## Future route geometry

SCS navigation distance/time/limit do not describe the remaining path. The web navigator calculates an independent route on extracted directed roads/prefabs, labelled HaulSense. It can also resolve an optional ETS2LA route-file UID sequence through the same graph, labelled Game GPS only when fresh and fully mapped. This is a separate provider, not an additional SDK channel. Provider installation and live Proton operation are not implied by fixture tests. See [web navigator](WEB-NAVIGATOR.md).

## Job completion and signals

V7 captures typed SDK gameplay attributes for job delivery/cancellation, fines, tolls and ferry/train payments, preserving presence flags. The native journal distinguishes official game job distance/time/revenue/XP from its observed distance/fuel/speed/route segment. Traffic lights remain outside the SCS telemetry channels; their live states come from the separate [ETS2LA provider](SIGNAL-PROVIDER.md).
