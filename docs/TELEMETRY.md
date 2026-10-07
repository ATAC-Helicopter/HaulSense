# SDK telemetry coverage

Source: [official SCS Telemetry SDK 1.14](https://modding.scssoft.com/wiki/Documentation/Engine/SDK/Telemetry), vendored under `ats-dualsense/third_party/scs-sdk` with its license. The registry below is the adapter contract, not a claim that every mod/truck/game version supplies every channel.

78 scalar/vector subscriptions plus the v6 world-placement subscription, plus per-wheel suspension/contact/angular velocity (up to 16), truck fuel/AdBlue capacities/RPM limit/name/brand and job cargo/mass/origin/destination. SDK frame end, pause, start, configuration and gameplay events are subscribed. All scalar/vector registrations request unavailable-value callbacks and report null to the inspector when missing.

Native units: speed and speed limit m/s; navigation distance metres; navigation time seconds; fuel/AdBlue litres; fuel consumption L/km; fuel range and odometer km; pressure psi; temperatures Celsius; battery volts; wear [0,1]; wheel deflection metres; angular velocity/acceleration quantities radians-based; placement heading/pitch/roll are turn fractions. Gear/retarder/aux-light states retain SDK integer values. Gameplay events are short acknowledgements, not an automated driving system.

The cockpit does not display all subscriptions in dedicated gauges; SDK inspector exposes the full stored packet. Primary trailer only. World placement feeds the web navigator with v6. Detailed shifter selectors, secondary trailer trains and audio haptics are deferred. Wiper state does not expose exact blade phase; the optional rhythm is synthetic. The SDK does not provide an ABS-engagement channel used here.

| Field | SDK channel | Wire type |
|---|---|---|
| `speed_mps` | `truck.speed` | float |
| `rpm` | `truck.engine.rpm` | float |
| `throttle` | `truck.effective.throttle` | float |
| `brake` | `truck.effective.brake` | float |
| `clutch` | `truck.effective.clutch` | float |
| `steering` | `truck.effective.steering` | float |
| `fuel` | `truck.fuel.amount` | float |
| `brake_air_pressure` | `truck.brake.air.pressure` | float |
| `brake_temperature` | `truck.brake.temperature` | float |
| `oil_pressure` | `truck.oil.pressure` | float |
| `water_temperature` | `truck.water.temperature` | float |
| `battery_voltage` | `truck.battery.voltage` | float |
| `nav_speed_limit` | `truck.navigation.speed.limit` | float |
| `nav_distance` | `truck.navigation.distance` | float |
| `nav_time` | `truck.navigation.time` | float |
| `fuel_consumption` | `truck.fuel.consumption.average` | float |
| `fuel_range` | `truck.fuel.range` | float |
| `adblue` | `truck.adblue` | float |
| `adblue_consumption` | `truck.adblue.consumption.average` | float |
| `oil_temperature` | `truck.oil.temperature` | float |
| `odometer` | `truck.odometer` | float |
| `cruise_speed` | `truck.cruise_control` | float |
| `dashboard_backlight` | `truck.dashboard.backlight` | float |
| `input_throttle` | `truck.input.throttle` | float |
| `input_brake` | `truck.input.brake` | float |
| `input_clutch` | `truck.input.clutch` | float |
| `input_steering` | `truck.input.steering` | float |
| `wear_engine` | `truck.wear.engine` | float |
| `wear_transmission` | `truck.wear.transmission` | float |
| `wear_cabin` | `truck.wear.cabin` | float |
| `wear_chassis` | `truck.wear.chassis` | float |
| `wear_wheels` | `truck.wear.wheels` | float |
| `cargo_damage` | `job.cargo.damage` | float |
| `trailer_wear_body` | `trailer.wear.body` | float |
| `trailer_wear_chassis` | `trailer.wear.chassis` | float |
| `trailer_wear_wheels` | `trailer.wear.wheels` | float |
| `gear` | `truck.engine.gear` | s32 |
| `displayed_gear` | `truck.displayed.gear` | s32 |
| `hshifter_slot` | `truck.hshifter.slot` | u32 |
| `retarder_level` | `truck.brake.retarder` | u32 |
| `aux_front` | `truck.light.aux.front` | u32 |
| `aux_roof` | `truck.light.aux.roof` | u32 |
| `engine_enabled` | `truck.engine.enabled` | bool |
| `electric_enabled` | `truck.electric.enabled` | bool |
| `parking_brake` | `truck.brake.parking` | bool |
| `left_blinker` | `truck.lblinker` | bool |
| `right_blinker` | `truck.rblinker` | bool |
| `left_blinker_light` | `truck.light.lblinker` | bool |
| `right_blinker_light` | `truck.light.rblinker` | bool |
| `hazards` | `truck.hazard.warning` | bool |
| `parking_lights` | `truck.light.parking` | bool |
| `low_beam` | `truck.light.beam.low` | bool |
| `high_beam` | `truck.light.beam.high` | bool |
| `beacon` | `truck.light.beacon` | bool |
| `brake_light` | `truck.light.brake` | bool |
| `reverse_light` | `truck.light.reverse` | bool |
| `wipers` | `truck.wipers` | bool |
| `fuel_warning` | `truck.fuel.warning` | bool |
| `air_warning` | `truck.brake.air.pressure.warning` | bool |
| `air_emergency` | `truck.brake.air.pressure.emergency` | bool |
| `oil_warning` | `truck.oil.pressure.warning` | bool |
| `water_warning` | `truck.water.temperature.warning` | bool |
| `battery_warning` | `truck.battery.voltage.warning` | bool |
| `engine_brake` | `truck.brake.motor` | bool |
| `differential_lock` | `truck.differential_lock` | bool |
| `adblue_warning` | `truck.adblue.warning` | bool |
| `lift_axle` | `truck.lift_axle` | bool |
| `lift_axle_indicator` | `truck.lift_axle.indicator` | bool |
| `trailer_lift_axle` | `truck.trailer.lift_axle` | bool |
| `trailer_lift_axle_indicator` | `truck.trailer.lift_axle.indicator` | bool |
| `trailer_connected` | `trailer.connected` | bool |
| `accel` | `truck.local.acceleration.linear` | vec |
| `velocity` | `truck.local.velocity.linear` | vec |
| `angular_velocity` | `truck.local.velocity.angular` | vec |
| `angular_acceleration` | `truck.local.acceleration.angular` | vec |
| `cabin_angvel` | `truck.cabin.velocity.angular` | vec |
| `cabin_angacc` | `truck.cabin.acceleration.angular` | vec |
| `trailer_accel` | `trailer.acceleration.linear` | vec |

| `world_position`, `heading`, `pitch`, `roll` | `truck.world.placement` | v6: 3 doubles + 3 floats, explicit availability |
| `game` | SDK initialization `game_id` | v6: byte enum (ATS / ETS2 / unknown) |

## V7 job extension

V7 is 898 bytes: the unchanged 863-byte v6 prefix plus 35 bytes. Fields: job-active byte; uint32 job-configuration sequence; uint32 gameplay attribute mask; int64 monetary amount; int32 XP; float32 job distance/cargo damage; uint32 game delivery minutes; autopark/autoload bytes. Unknown event fields have clear mask bits and never become measured zeros. Gameplay callbacks send immediately so the native journal can consume each event independently from dashboard polling. V4/v5/v6 remain exact-size compatible with the extension absent.
