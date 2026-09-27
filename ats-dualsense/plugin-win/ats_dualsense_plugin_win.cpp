// HaulSense: freestanding Windows x64 plugin. Only imports Winsock and kernel clock.
#include "../third_party/scs-sdk/include/scssdk_telemetry.h"
#include "../daemon/src/protocol.hpp"
extern "C" {
__declspec(dllimport) int __stdcall WSAStartup(unsigned short,void*);
__declspec(dllimport) int __stdcall WSACleanup(void);
__declspec(dllimport) unsigned long long __stdcall socket(int,int,int);
__declspec(dllimport) int __stdcall sendto(unsigned long long,const char*,int,int,const void*,int);
__declspec(dllimport) int __stdcall closesocket(unsigned long long);
__declspec(dllimport) unsigned long long __stdcall GetTickCount64(void);
int _fltused=0;
void* memset(void* dst,int value,unsigned long long count){auto*p=(unsigned char*)dst;while(count--)*p++=(unsigned char)value;return dst;}
void* memcpy(void* dst,const void* src,unsigned long long count){auto*d=(unsigned char*)dst;auto*s=(const unsigned char*)src;while(count--)*d++=*s++;return dst;}
}
struct Address {short family; unsigned short port; unsigned int addr; char zero[8];};
struct Binding {void* target; ChannelId id; unsigned type;};
static Binding bindings[CHANNEL_COUNT]{};
static AtsTelemetryPacket g{};
static scs_log_t game_log{};
static scs_telemetry_register_for_channel_t register_channel{};
static unsigned long long sock=~0ull,last_send{};
static Address dst{};
static unsigned registered_wheels{};
static bool paused=true;
static bool same(const char*a,const char*b){if(!a||!b)return false;while(*a&&*a==*b){++a;++b;}return *a==*b;}
static void copy_text(char*dst,unsigned cap,const char*src){unsigned i=0;if(src)while(i+1<cap&&src[i]){dst[i]=src[i];++i;}dst[i]=0;}
static void mark(ChannelId id,bool on){auto&mask=g.available[id/64];if(on)mask|=1ull<<(id%64);else mask&=~(1ull<<(id%64));}
static void SCSAPIFUNC on_value(const char*,unsigned,const scs_value_t*v,void*ctx){
    auto&b=*(Binding*)ctx;mark(b.id,v!=nullptr);if(!v){memset(b.target,0,b.type==SCS_VALUE_TYPE_fvector?sizeof(WireVector):b.type==SCS_VALUE_TYPE_bool?1:4);return;}
    switch(v->type){
    case SCS_VALUE_TYPE_float: *(float*)b.target=v->value_float.value;break;
    case SCS_VALUE_TYPE_bool: *(unsigned char*)b.target=v->value_bool.value?1:0;break;
    case SCS_VALUE_TYPE_s32: *(int*)b.target=v->value_s32.value;break;
    case SCS_VALUE_TYPE_u32: *(unsigned*)b.target=v->value_u32.value;break;
    case SCS_VALUE_TYPE_fvector: {auto&t=*(WireVector*)b.target;t={v->value_fvector.x,v->value_fvector.y,v->value_fvector.z};break;}
    }
}
static void SCSAPIFUNC on_wheel(const char*name,unsigned index,const scs_value_t*v,void*){
    if(index>=16)return;
    unsigned char bit=same(name,"truck.wheel.on_ground")?2:same(name,"truck.wheel.angular_velocity")?4:1;
    if(!v){g.wheel_available[index]&=~bit;return;}g.wheel_available[index]|=bit;
    if(bit==1)g.wheel_suspension[index]=v->value_float.value;
    else if(bit==2)g.wheel_ground[index]=v->value_bool.value;
    else g.wheel_velocity[index]=v->value_float.value;
}
static void wheels(unsigned count){
    if(count>16)count=16;
    for(unsigned i=registered_wheels;i<count;++i){
        register_channel("truck.wheel.suspension.deflection",i,SCS_VALUE_TYPE_float,SCS_TELEMETRY_CHANNEL_FLAG_no_value,on_wheel,nullptr);
        register_channel("truck.wheel.on_ground",i,SCS_VALUE_TYPE_bool,SCS_TELEMETRY_CHANNEL_FLAG_no_value,on_wheel,nullptr);
        register_channel("truck.wheel.angular_velocity",i,SCS_VALUE_TYPE_float,SCS_TELEMETRY_CHANNEL_FLAG_no_value,on_wheel,nullptr);
    }
    if(count>registered_wheels)registered_wheels=count;
}
static void send_packet(bool force=false){
    if(sock==~0ull)return;
    auto now=GetTickCount64();if(!force&&now-last_send<20)return;last_send=now; // <=50 Hz independent of game FPS
    float sum=0,vel=0,mn=1e6f,mx=-1e6f;unsigned valid=0,on=0,ground_count=0,vel_count=0;
    for(unsigned i=0;i<g.wheel_count&&i<16;++i){
        if(g.wheel_available[i]&1){float s=g.wheel_suspension[i];sum+=s;mn=s<mn?s:mn;mx=s>mx?s:mx;++valid;}
        if(g.wheel_available[i]&2){on+=g.wheel_ground[i]!=0;++ground_count;}
        if(g.wheel_available[i]&4){float v=g.wheel_velocity[i];vel+=v<0?-v:v;++vel_count;}
    }
    g.suspension_average=valid?sum/valid:0;g.suspension_spread=valid?mx-mn:0;
    g.wheel_ground_ratio=ground_count?(float)on/ground_count:1;g.wheel_angular_velocity=vel_count?vel/vel_count:0;
    float wear[]={g.wear_engine,g.wear_transmission,g.wear_cabin,g.wear_chassis,g.wear_wheels};g.max_wear=0;
    for(float v:wear)if(v>g.max_wear)g.max_wear=v;
    g.damage_warning=g.max_wear>=.65f;g.damage_critical=g.max_wear>=.85f;
    g.cruise=channel_available(g,CHANNEL_cruise_speed)&&g.cruise_speed>.1f;g.paused=paused;
    ++g.sequence;sendto(sock,(const char*)&g,sizeof(g),0,&dst,sizeof(dst));
}
static void SCSAPIFUNC on_event(unsigned e,const void*info,void*){
    if(e==SCS_TELEMETRY_EVENT_frame_end){if(!paused)send_packet();return;}
    if(e==SCS_TELEMETRY_EVENT_paused){paused=true;send_packet(true);return;}
    if(e==SCS_TELEMETRY_EVENT_started){paused=false;send_packet(true);return;}
    if(e==SCS_TELEMETRY_EVENT_gameplay&&info){
        auto&event=*(const scs_telemetry_gameplay_event_t*)info;
        copy_text(g.last_event,sizeof(g.last_event),event.id);++g.event_sequence;return;
    }
    if(e!=SCS_TELEMETRY_EVENT_configuration||!info)return;
    auto&cfg=*(const scs_telemetry_configuration_t*)info;if(!cfg.attributes)return;
    bool truck=same(cfg.id,"truck"),job=same(cfg.id,"job");
    if(truck){g.truck_name[0]=g.truck_brand[0]=0;g.wheel_count=0;g.rpm_limit=2500;g.fuel_capacity=1;g.adblue_capacity=0;for(unsigned i=0;i<16;++i)g.wheel_available[i]=0;}
    if(job){g.cargo[0]=g.origin[0]=g.destination[0]=0;g.cargo_mass=0;}
    for(auto*a=cfg.attributes;a->name;++a){
        auto&v=a->value;
        if(truck&&v.type==SCS_VALUE_TYPE_float){
            if(same(a->name,"fuel.capacity"))g.fuel_capacity=v.value_float.value;
            if(same(a->name,"adblue.capacity"))g.adblue_capacity=v.value_float.value;
            if(same(a->name,"rpm.limit"))g.rpm_limit=v.value_float.value;
        }
        if(truck&&v.type==SCS_VALUE_TYPE_u32&&same(a->name,"wheels.count")){g.wheel_count=v.value_u32.value;wheels(g.wheel_count);}
        if(job&&v.type==SCS_VALUE_TYPE_float&&same(a->name,"cargo.mass"))g.cargo_mass=v.value_float.value;
        if(v.type==SCS_VALUE_TYPE_string){const char*s=v.value_string.value;
            if(truck&&same(a->name,"name"))copy_text(g.truck_name,sizeof(g.truck_name),s);
            if(truck&&same(a->name,"brand"))copy_text(g.truck_brand,sizeof(g.truck_brand),s);
            if(job&&same(a->name,"cargo"))copy_text(g.cargo,sizeof(g.cargo),s);
            if(job&&same(a->name,"source.city"))copy_text(g.origin,sizeof(g.origin),s);
            if(job&&same(a->name,"destination.city"))copy_text(g.destination,sizeof(g.destination),s);
        }
    }
}
extern "C" SCSAPI_RESULT scs_telemetry_init(unsigned version,const scs_telemetry_init_params_t*params){
    if(version!=SCS_TELEMETRY_VERSION_1_01&&version!=SCS_TELEMETRY_VERSION_1_00)return SCS_RESULT_unsupported;
    if(!params)return SCS_RESULT_invalid_parameter;
    auto&p=*(const scs_telemetry_init_params_v100_t*)params;game_log=p.common.log;register_channel=p.register_for_channel;
    if(!register_channel||!p.register_for_event)return SCS_RESULT_invalid_parameter;
    alignas(8)unsigned char wsa[512]{};if(WSAStartup(0x0202,wsa))return SCS_RESULT_generic_error;
    sock=socket(2,2,17);if(sock==~0ull){WSACleanup();return SCS_RESULT_generic_error;}
    dst={2,0x8f98,0x0100007f,{}};g={};paused=true;last_send=0;registered_wheels=0;
    for(unsigned e=2;e<=6;++e)if(p.register_for_event(e,on_event,nullptr)!=SCS_RESULT_ok){closesocket(sock);sock=~0ull;WSACleanup();return SCS_RESULT_generic_error;}
#define REGISTER(n,c,type) bindings[CHANNEL_##n]={&g.n,CHANNEL_##n,type}; if(register_channel(c,SCS_U32_NIL,type,SCS_TELEMETRY_CHANNEL_FLAG_no_value,on_value,&bindings[CHANNEL_##n])!=SCS_RESULT_ok&&game_log)game_log(SCS_LOG_TYPE_warning,"HaulSense: optional channel unavailable: " c);
#define TELE_FLOAT(n,c) REGISTER(n,c,SCS_VALUE_TYPE_float)
#define TELE_BOOL(n,c) REGISTER(n,c,SCS_VALUE_TYPE_bool)
#define TELE_U32(n,c) REGISTER(n,c,SCS_VALUE_TYPE_u32)
#define TELE_S32(n,c) REGISTER(n,c,SCS_VALUE_TYPE_s32)
#define TELE_VEC(n,c) REGISTER(n,c,SCS_VALUE_TYPE_fvector)
#include "../daemon/src/channels.inc"
#undef TELE_FLOAT
#undef TELE_BOOL
#undef TELE_U32
#undef TELE_S32
#undef TELE_VEC
#undef REGISTER
    if(game_log)game_log(SCS_LOG_TYPE_message,"HaulSense 0.8.0: SDK 1.14 telemetry ready (50 Hz ceiling)");return SCS_RESULT_ok;
}
extern "C" SCSAPI_VOID scs_telemetry_shutdown(){paused=true;send_packet(true);if(sock!=~0ull)closesocket(sock);sock=~0ull;WSACleanup();game_log=nullptr;}
