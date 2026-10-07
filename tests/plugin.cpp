// Native harness for the exact freestanding plugin source and official SDK ABI.
#include "../ats-dualsense/third_party/scs-sdk/include/common/scssdk_telemetry_common_configs.h"
#define __declspec(x)
#define __stdcall
#include "../ats-dualsense/plugin-win/ats_dualsense_plugin_win.cpp"
extern "C" void abort();
extern "C" int puts(const char*);
#define CHECK(x) do { if(!(x)){puts("FAIL: " #x);abort();} } while(0)
static unsigned long long fake_time=1000;
static int sent=0;static AtsTelemetryPacket captured;
extern "C" int WSAStartup(unsigned short,void*){return 0;}
extern "C" int WSACleanup(){return 0;}
extern "C" unsigned long long socket(int,int,int){return 5;}
extern "C" int closesocket(unsigned long long){return 0;}
extern "C" unsigned long long GetTickCount64(){return fake_time;}
extern "C" int sendto(unsigned long long,const char*data,int size,int,const void*,int){CHECK(size==(int)sizeof(captured));memcpy(&captured,data,size);++sent;return size;}
static scs_result_t register_event(unsigned,scs_telemetry_event_callback_t,void*){return SCS_RESULT_ok;}
static scs_result_t channel(const char*name,unsigned,scs_value_type_t,unsigned,scs_telemetry_channel_callback_t,void*){
    if(same(name,"truck.fuel.range"))return SCS_RESULT_not_found;return SCS_RESULT_ok;
}
int main(){
    scs_telemetry_init_params_v100_t params{};params.common.game_id="ats";params.register_for_channel=channel;params.register_for_event=register_event;
    CHECK(scs_telemetry_init(SCS_TELEMETRY_VERSION_1_01,&params)==SCS_RESULT_ok);
    CHECK(g.game==1);CHECK(!channel_available(g,CHANNEL_speed_mps));
    scs_value_t speed{};speed.type=SCS_VALUE_TYPE_float;speed.value_float.value=22;
    on_value("truck.speed",SCS_U32_NIL,&speed,&bindings[CHANNEL_speed_mps]);CHECK(g.speed_mps==22&&channel_available(g,CHANNEL_speed_mps));
    scs_value_t placement{};placement.type=SCS_VALUE_TYPE_dplacement;placement.value_dplacement.position={12345.125,42,-6789.5};placement.value_dplacement.orientation={.75f,.01f,-.02f};
    on_placement("truck.world.placement",SCS_U32_NIL,&placement,nullptr);CHECK(g.placement_available&&g.world_x==12345.125&&g.heading==.75f);
    on_placement("truck.world.placement",SCS_U32_NIL,nullptr,nullptr);CHECK(!g.placement_available&&g.world_x==0);
    scs_value_t acceleration{};acceleration.type=SCS_VALUE_TYPE_fvector;acceleration.value_fvector={1,2,3};
    on_value("truck.local.acceleration.linear",SCS_U32_NIL,&acceleration,&bindings[CHANNEL_accel]);CHECK(g.accel.x==1&&g.accel.y==2&&g.accel.z==3);
    on_event(SCS_TELEMETRY_EVENT_started,nullptr,nullptr);CHECK(sent==1&&!captured.paused&&captured.speed_mps==22);
    on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==1);
    fake_time+=20;on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==2);
    on_event(SCS_TELEMETRY_EVENT_paused,nullptr,nullptr);CHECK(sent==3&&captured.paused);
    fake_time+=100;on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==3);
    on_value("truck.speed",SCS_U32_NIL,nullptr,&bindings[CHANNEL_speed_mps]);CHECK(!channel_available(g,CHANNEL_speed_mps));
    scs_named_value_t attrs[3]{};attrs[0].name="name";attrs[0].value.type=SCS_VALUE_TYPE_string;attrs[0].value.value_string.value="W900";
    attrs[1].name="fuel.capacity";attrs[1].value.type=SCS_VALUE_TYPE_float;attrs[1].value.value_float.value=450;
    scs_telemetry_configuration_t config{"truck",attrs};on_event(SCS_TELEMETRY_EVENT_configuration,&config,nullptr);CHECK(same(g.truck_name,"W900")&&g.fuel_capacity==450);
    scs_named_value_t job[4]{};
    job[0].name=SCS_TELEMETRY_CONFIG_ATTRIBUTE_destination_city;job[0].value.type=SCS_VALUE_TYPE_string;job[0].value.value_string.value="Albuquerque";
    job[1].name=SCS_TELEMETRY_CONFIG_ATTRIBUTE_source_city;job[1].value.type=SCS_VALUE_TYPE_string;job[1].value.value_string.value="Flagstaff";
    job[2].name=SCS_TELEMETRY_CONFIG_ATTRIBUTE_cargo_mass;job[2].value.type=SCS_VALUE_TYPE_float;job[2].value.value_float.value=12000;
    config={SCS_TELEMETRY_CONFIG_job,job};on_event(SCS_TELEMETRY_EVENT_configuration,&config,nullptr);
    CHECK(same(g.destination,"Albuquerque")&&same(g.origin,"Flagstaff")&&g.cargo_mass==12000);
    scs_named_value_t empty[1]{};config={SCS_TELEMETRY_CONFIG_job,empty};on_event(SCS_TELEMETRY_EVENT_configuration,&config,nullptr);
    CHECK(!g.destination[0]&&!g.origin[0]&&!g.cargo[0]&&g.cargo_mass==0);
    scs_named_value_t delivery[4]{};
    delivery[0].name="revenue";delivery[0].value.type=SCS_VALUE_TYPE_s64;delivery[0].value.value_s64.value=123456;
    delivery[1].name="earned.xp";delivery[1].value.type=SCS_VALUE_TYPE_s32;delivery[1].value.value_s32.value=42;
    delivery[2].name="auto.park.used";delivery[2].value.type=SCS_VALUE_TYPE_bool;delivery[2].value.value_bool.value=0;
    scs_telemetry_gameplay_event_t event{"job.delivered",delivery};
    on_event(SCS_TELEMETRY_EVENT_gameplay,&event,nullptr);
    CHECK(g.event_money==123456&&g.event_xp==42&&(g.event_attributes&EVENT_AUTOPARK)&&!g.event_autopark);
    CHECK(captured.event_sequence==g.event_sequence&&captured.event_money==123456);
    scs_telemetry_shutdown();CHECK(captured.paused);
    puts("SDK ABI, availability, pause/resume, 50 Hz ceiling, truck configuration: passed");
}
