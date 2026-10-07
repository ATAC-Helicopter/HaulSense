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
    WireU16 version = 7;
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
    // v6 suffix. The v5 prefix and channel IDs remain unchanged.
    WireU8 placement_available{};
    double world_x{}, world_y{}, world_z{};
    float heading{}, pitch{}, roll{};
    WireU8 game{}; // 1 = ATS, 2 = ETS2, 0 = unknown

    // v7: explicit job lifecycle and typed gameplay attributes. Legacy prefixes remain fixed.
    WireU8 job_active{};
    WireU32 job_sequence{}, event_attributes{};
    long long event_money{};
    int event_xp{};
    float event_distance{}, event_cargo_damage{};
    WireU32 event_game_minutes{};
    WireU8 event_autopark{}, event_autoload{};
};
#pragma pack(pop)
constexpr unsigned V6_PACKET_SIZE = 863;
constexpr unsigned V5_PACKET_SIZE = 825;
enum EventAttribute { EVENT_MONEY=1, EVENT_XP=2, EVENT_DISTANCE=4, EVENT_CARGO_DAMAGE=8, EVENT_GAME_MINUTES=16, EVENT_AUTOPARK=32, EVENT_AUTOLOAD=64 };
static_assert(sizeof(AtsTelemetryPacket)==898,"v7 ABI changed");
static_assert(sizeof(WireU64) == 8 && sizeof(float) == 4 && sizeof(double) == 8);
static_assert(V5_PACKET_SIZE == 825, "Do not alter the v5 prefix");
static_assert(CHANNEL_COUNT <= 128);
static_assert(sizeof(AtsTelemetryPacket) < 1400, "Must fit one LAN MTU");
inline bool channel_available(const AtsTelemetryPacket& t, ChannelId id) {
    return (t.available[id / 64] & (1ull << (id % 64))) != 0;
}
