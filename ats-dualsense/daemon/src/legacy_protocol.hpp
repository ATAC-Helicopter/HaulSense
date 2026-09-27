#pragma once
#include <cstdint>
#pragma pack(push,1)
struct LegacyTelemetryPacket {
    uint32_t magic = 0x41545344; // ATSD
    uint16_t version = 4;
    uint16_t size = sizeof(LegacyTelemetryPacket);
    uint64_t sequence = 0;

    float speed_mps = 0;
    float rpm = 0;
    float rpm_limit = 2500;
    float throttle = 0;
    float brake = 0;
    float clutch = 0;
    float steering = 0;
    float fuel = 0;
    float fuel_capacity = 1;
    float brake_air_pressure = 0;
    float brake_temperature = 0;
    float oil_pressure = 0;
    float water_temperature = 0;
    float battery_voltage = 0;
    float accel_x = 0, accel_y = 0, accel_z = 0;
    float cabin_angvel_x = 0, cabin_angvel_y = 0, cabin_angvel_z = 0;
    float cabin_angacc_x = 0, cabin_angacc_y = 0, cabin_angacc_z = 0;
    float nav_speed_limit = 0;
    float max_wear = 0;

    // Aggregated wheel data from SCS indexed channels.
    float suspension_average = 0;
    float suspension_spread = 0;
    float wheel_ground_ratio = 1;
    float wheel_angular_velocity = 0;

    int32_t gear = 0;
    uint32_t retarder_level = 0;
    uint32_t wheel_count = 0;

    uint8_t engine_enabled = 0;
    uint8_t electric_enabled = 0;
    uint8_t parking_brake = 0;
    uint8_t left_blinker = 0;
    uint8_t right_blinker = 0;
    uint8_t left_blinker_light = 0;
    uint8_t right_blinker_light = 0;
    uint8_t hazards = 0;
    uint8_t parking_lights = 0;
    uint8_t low_beam = 0;
    uint8_t high_beam = 0;
    uint8_t beacon = 0;
    uint8_t brake_light = 0;
    uint8_t reverse_light = 0;
    uint8_t wipers = 0;
    uint8_t fuel_warning = 0;
    uint8_t air_warning = 0;
    uint8_t air_emergency = 0;
    uint8_t oil_warning = 0;
    uint8_t water_warning = 0;
    uint8_t battery_warning = 0;
    uint8_t cruise = 0;
    uint8_t engine_brake = 0;
    uint8_t differential_lock = 0;
    uint8_t damage_warning = 0;
    uint8_t damage_critical = 0;
    uint8_t reserved[5]{};
};
#pragma pack(pop)
static_assert(sizeof(LegacyTelemetryPacket) == 175);

inline AtsTelemetryPacket convert_legacy(const LegacyTelemetryPacket& old){
    AtsTelemetryPacket t;
    t.sequence=old.sequence;
    t.speed_mps=old.speed_mps; t.available[CHANNEL_speed_mps/64]|=1ull<<(CHANNEL_speed_mps%64);
    t.rpm=old.rpm; t.available[CHANNEL_rpm/64]|=1ull<<(CHANNEL_rpm%64);
    t.throttle=old.throttle; t.available[CHANNEL_throttle/64]|=1ull<<(CHANNEL_throttle%64);
    t.brake=old.brake; t.available[CHANNEL_brake/64]|=1ull<<(CHANNEL_brake%64);
    t.clutch=old.clutch; t.available[CHANNEL_clutch/64]|=1ull<<(CHANNEL_clutch%64);
    t.steering=old.steering; t.available[CHANNEL_steering/64]|=1ull<<(CHANNEL_steering%64);
    t.fuel=old.fuel; t.available[CHANNEL_fuel/64]|=1ull<<(CHANNEL_fuel%64);
    t.brake_air_pressure=old.brake_air_pressure; t.available[CHANNEL_brake_air_pressure/64]|=1ull<<(CHANNEL_brake_air_pressure%64);
    t.brake_temperature=old.brake_temperature; t.available[CHANNEL_brake_temperature/64]|=1ull<<(CHANNEL_brake_temperature%64);
    t.oil_pressure=old.oil_pressure; t.available[CHANNEL_oil_pressure/64]|=1ull<<(CHANNEL_oil_pressure%64);
    t.water_temperature=old.water_temperature; t.available[CHANNEL_water_temperature/64]|=1ull<<(CHANNEL_water_temperature%64);
    t.battery_voltage=old.battery_voltage; t.available[CHANNEL_battery_voltage/64]|=1ull<<(CHANNEL_battery_voltage%64);
    t.nav_speed_limit=old.nav_speed_limit; t.available[CHANNEL_nav_speed_limit/64]|=1ull<<(CHANNEL_nav_speed_limit%64);
    t.gear=old.gear; t.available[CHANNEL_gear/64]|=1ull<<(CHANNEL_gear%64);
    t.retarder_level=old.retarder_level; t.available[CHANNEL_retarder_level/64]|=1ull<<(CHANNEL_retarder_level%64);
    t.engine_enabled=old.engine_enabled; t.available[CHANNEL_engine_enabled/64]|=1ull<<(CHANNEL_engine_enabled%64);
    t.electric_enabled=old.electric_enabled; t.available[CHANNEL_electric_enabled/64]|=1ull<<(CHANNEL_electric_enabled%64);
    t.parking_brake=old.parking_brake; t.available[CHANNEL_parking_brake/64]|=1ull<<(CHANNEL_parking_brake%64);
    t.left_blinker=old.left_blinker; t.available[CHANNEL_left_blinker/64]|=1ull<<(CHANNEL_left_blinker%64);
    t.right_blinker=old.right_blinker; t.available[CHANNEL_right_blinker/64]|=1ull<<(CHANNEL_right_blinker%64);
    t.left_blinker_light=old.left_blinker_light; t.available[CHANNEL_left_blinker_light/64]|=1ull<<(CHANNEL_left_blinker_light%64);
    t.right_blinker_light=old.right_blinker_light; t.available[CHANNEL_right_blinker_light/64]|=1ull<<(CHANNEL_right_blinker_light%64);
    t.hazards=old.hazards; t.available[CHANNEL_hazards/64]|=1ull<<(CHANNEL_hazards%64);
    t.parking_lights=old.parking_lights; t.available[CHANNEL_parking_lights/64]|=1ull<<(CHANNEL_parking_lights%64);
    t.low_beam=old.low_beam; t.available[CHANNEL_low_beam/64]|=1ull<<(CHANNEL_low_beam%64);
    t.high_beam=old.high_beam; t.available[CHANNEL_high_beam/64]|=1ull<<(CHANNEL_high_beam%64);
    t.beacon=old.beacon; t.available[CHANNEL_beacon/64]|=1ull<<(CHANNEL_beacon%64);
    t.brake_light=old.brake_light; t.available[CHANNEL_brake_light/64]|=1ull<<(CHANNEL_brake_light%64);
    t.reverse_light=old.reverse_light; t.available[CHANNEL_reverse_light/64]|=1ull<<(CHANNEL_reverse_light%64);
    t.wipers=old.wipers; t.available[CHANNEL_wipers/64]|=1ull<<(CHANNEL_wipers%64);
    t.fuel_warning=old.fuel_warning; t.available[CHANNEL_fuel_warning/64]|=1ull<<(CHANNEL_fuel_warning%64);
    t.air_warning=old.air_warning; t.available[CHANNEL_air_warning/64]|=1ull<<(CHANNEL_air_warning%64);
    t.air_emergency=old.air_emergency; t.available[CHANNEL_air_emergency/64]|=1ull<<(CHANNEL_air_emergency%64);
    t.oil_warning=old.oil_warning; t.available[CHANNEL_oil_warning/64]|=1ull<<(CHANNEL_oil_warning%64);
    t.water_warning=old.water_warning; t.available[CHANNEL_water_warning/64]|=1ull<<(CHANNEL_water_warning%64);
    t.battery_warning=old.battery_warning; t.available[CHANNEL_battery_warning/64]|=1ull<<(CHANNEL_battery_warning%64);
    t.engine_brake=old.engine_brake; t.available[CHANNEL_engine_brake/64]|=1ull<<(CHANNEL_engine_brake%64);
    t.differential_lock=old.differential_lock; t.available[CHANNEL_differential_lock/64]|=1ull<<(CHANNEL_differential_lock%64);
    t.accel={old.accel_x,old.accel_y,old.accel_z};t.available[CHANNEL_accel/64]|=1ull<<(CHANNEL_accel%64);
    t.cabin_angvel={old.cabin_angvel_x,old.cabin_angvel_y,old.cabin_angvel_z};t.available[CHANNEL_cabin_angvel/64]|=1ull<<(CHANNEL_cabin_angvel%64);
    t.cabin_angacc={old.cabin_angacc_x,old.cabin_angacc_y,old.cabin_angacc_z};t.available[CHANNEL_cabin_angacc/64]|=1ull<<(CHANNEL_cabin_angacc%64);
    t.rpm_limit=old.rpm_limit;
    t.fuel_capacity=old.fuel_capacity;
    t.suspension_average=old.suspension_average;
    t.suspension_spread=old.suspension_spread;
    t.wheel_ground_ratio=old.wheel_ground_ratio;
    t.wheel_angular_velocity=old.wheel_angular_velocity;
    t.max_wear=old.max_wear;
    t.wheel_count=old.wheel_count;
    t.cruise=old.cruise;
    t.damage_warning=old.damage_warning;
    t.damage_critical=old.damage_critical;
    t.displayed_gear=old.gear;t.available[CHANNEL_displayed_gear/64]|=1ull<<(CHANNEL_displayed_gear%64);
    return t;
}
