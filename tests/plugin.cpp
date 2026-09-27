// Native harness for the exact freestanding plugin source and official SDK ABI.
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
    scs_telemetry_init_params_v100_t params{};params.register_for_channel=channel;params.register_for_event=register_event;
    CHECK(scs_telemetry_init(SCS_TELEMETRY_VERSION_1_01,&params)==SCS_RESULT_ok);
    CHECK(!channel_available(g,CHANNEL_speed_mps));
    scs_value_t speed{};speed.type=SCS_VALUE_TYPE_float;speed.value_float.value=22;
    on_value("truck.speed",SCS_U32_NIL,&speed,&bindings[CHANNEL_speed_mps]);CHECK(g.speed_mps==22&&channel_available(g,CHANNEL_speed_mps));
    on_event(SCS_TELEMETRY_EVENT_started,nullptr,nullptr);CHECK(sent==1&&!captured.paused&&captured.speed_mps==22);
    on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==1);
    fake_time+=20;on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==2);
    on_event(SCS_TELEMETRY_EVENT_paused,nullptr,nullptr);CHECK(sent==3&&captured.paused);
    fake_time+=100;on_event(SCS_TELEMETRY_EVENT_frame_end,nullptr,nullptr);CHECK(sent==3);
    on_value("truck.speed",SCS_U32_NIL,nullptr,&bindings[CHANNEL_speed_mps]);CHECK(!channel_available(g,CHANNEL_speed_mps));
    scs_named_value_t attrs[3]{};attrs[0].name="name";attrs[0].value.type=SCS_VALUE_TYPE_string;attrs[0].value.value_string.value="W900";
    attrs[1].name="fuel.capacity";attrs[1].value.type=SCS_VALUE_TYPE_float;attrs[1].value.value_float.value=450;
    scs_telemetry_configuration_t config{"truck",attrs};on_event(SCS_TELEMETRY_EVENT_configuration,&config,nullptr);CHECK(same(g.truck_name,"W900")&&g.fuel_capacity==450);
    scs_telemetry_shutdown();CHECK(captured.paused);
    puts("SDK ABI, availability, pause/resume, 50 Hz ceiling, truck configuration: passed");
}
