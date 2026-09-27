#pragma once
// Fixed little-endian wire ABI, shared by the freestanding PE DLL and Linux daemon.
using WireU8 = unsigned char;
using WireU16 = unsigned short;
using WireU32 = unsigned int;
using WireU64 = unsigned long long;
struct WireVector { float x{}, y{}, z{}; };
enum ChannelId {
#define TELE_FLOAT(n,c) CHANNEL_##n,
#define TELE_BOOL(n,c) CHANNEL_##n,
#define TELE_U32(n,c) CHANNEL_##n,
#define TELE_S32(n,c) CHANNEL_##n,
#define TELE_VEC(n,c) CHANNEL_##n,
#include "channels.inc"
#undef TELE_FLOAT
#undef TELE_BOOL
#undef TELE_U32
#undef TELE_S32
#undef TELE_VEC
CHANNEL_COUNT
};
#pragma pack(push,1)
struct AtsTelemetryPacket {
    WireU32 magic = 0x41545344;
    WireU16 version = 5;
    WireU16 size = sizeof(AtsTelemetryPacket);
    WireU64 sequence{};
    WireU64 available[2]{};
    WireU8 paused{};
#define TELE_FLOAT(n,c) float n{};
#define TELE_BOOL(n,c) WireU8 n{};
#define TELE_U32(n,c) WireU32 n{};
#define TELE_S32(n,c) int n{};
#define TELE_VEC(n,c) WireVector n{};
#include "channels.inc"
#undef TELE_FLOAT
#undef TELE_BOOL
#undef TELE_U32
#undef TELE_S32
#undef TELE_VEC
    float rpm_limit = 2500, fuel_capacity = 1, adblue_capacity{};
    float suspension_average{}, suspension_spread{}, wheel_ground_ratio = 1, wheel_angular_velocity{};
    float max_wear{}, cargo_mass{};
    WireU32 wheel_count{};
    WireU8 cruise{}, damage_warning{}, damage_critical{};
    float wheel_suspension[16]{}, wheel_velocity[16]{};
    WireU8 wheel_ground[16]{}, wheel_available[16]{};
    WireU32 event_sequence{};
    char last_event[48]{};
    char truck_name[64]{}, truck_brand[32]{}, cargo[64]{}, origin[48]{}, destination[48]{};
};
#pragma pack(pop)
static_assert(sizeof(WireU64) == 8 && sizeof(float) == 4);
static_assert(CHANNEL_COUNT <= 128);
static_assert(sizeof(AtsTelemetryPacket) < 1400, "Must fit one LAN MTU");
inline bool channel_available(const AtsTelemetryPacket& t, ChannelId id) {
    return (t.available[id / 64] & (1ull << (id % 64))) != 0;
}
