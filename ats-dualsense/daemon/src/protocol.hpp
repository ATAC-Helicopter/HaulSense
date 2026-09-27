#pragma once
#include <cstdint>
#pragma pack(push,1)
struct AtsTelemetryPacket {
    uint32_t magic = 0x41545344; // ATSD
    uint16_t version = 4;
    uint16_t size = sizeof(AtsTelemetryPacket);
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
static_assert(sizeof(AtsTelemetryPacket) == 175);
