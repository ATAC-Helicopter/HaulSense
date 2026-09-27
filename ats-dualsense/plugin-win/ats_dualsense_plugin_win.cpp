// ATS DualSense Bridge - Windows x64 SCS telemetry plugin for ATS under Proton.
// Self-contained implementation of the public SCS Telemetry SDK ABI subset used here.

extern "C" {
typedef unsigned char u8; typedef unsigned short u16; typedef unsigned int u32; typedef unsigned long long u64;
typedef signed int s32; typedef float f32; typedef const char* cstr; typedef void* ctx_t; typedef s32 result_t;
__declspec(dllimport) int __stdcall WSAStartup(u16,void*); __declspec(dllimport) int __stdcall WSACleanup(void);
typedef unsigned long long SOCKET_T; __declspec(dllimport) SOCKET_T __stdcall socket(int,int,int);
__declspec(dllimport) int __stdcall sendto(SOCKET_T,const char*,int,int,const void*,int); __declspec(dllimport) int __stdcall closesocket(SOCKET_T);
}
extern "C" int _fltused=0;
extern "C" void* memset(void* dst,int value,unsigned long long count){unsigned char*p=(unsigned char*)dst;for(unsigned long long i=0;i<count;++i)p[i]=(unsigned char)value;return dst;}

static constexpr result_t OK=0, UNSUPPORTED=-1, GENERIC=-7; static constexpr u32 NIL=0xffffffffu;
static constexpr u32 VER(u32 a,u32 b){return(a<<16)|b;} static constexpr u32 V100=VER(1,0),V101=VER(1,1);
static constexpr u32 T_BOOL=1,T_S32=2,T_U32=3,T_FLOAT=5,T_FVECTOR=7;
static constexpr u32 EVT_FRAME_END=2,EVT_PAUSED=3,EVT_STARTED=4,EVT_CONFIG=5;
static constexpr s32 LOG_MSG=0,LOG_WARN=1,LOG_ERR=2;

#pragma pack(push,1)
struct AtsTelemetryPacket{
 u32 magic;u16 version;u16 size;u64 sequence;
 f32 speed_mps,rpm,rpm_limit,throttle,brake,clutch,steering,fuel,fuel_capacity,brake_air_pressure,brake_temperature,oil_pressure,water_temperature,battery_voltage;
 f32 accel_x,accel_y,accel_z,cabin_angvel_x,cabin_angvel_y,cabin_angvel_z,cabin_angacc_x,cabin_angacc_y,cabin_angacc_z,nav_speed_limit,max_wear;
 f32 suspension_average,suspension_spread,wheel_ground_ratio,wheel_angular_velocity;
 s32 gear;u32 retarder_level;u32 wheel_count;
 u8 engine_enabled,electric_enabled,parking_brake,left_blinker,right_blinker,left_blinker_light,right_blinker_light,hazards,parking_lights,low_beam,high_beam,beacon,brake_light,reverse_light,wipers;
 u8 fuel_warning,air_warning,air_emergency,oil_warning,water_warning,battery_warning,cruise,engine_brake,differential_lock,damage_warning,damage_critical,reserved[5];
};
#pragma pack(pop)
static_assert(sizeof(AtsTelemetryPacket)==175,"wire layout changed");

struct value_bool{u8 value;}; struct value_s32{s32 value;}; struct value_u32{u32 value;}; struct value_float{f32 value;}; struct value_fvector{f32 x,y,z;};
struct value_t{u32 type;u32 _padding;union{value_bool value_bool_;value_s32 value_s32_;value_u32 value_u32_;value_float value_float_;value_fvector value_fvector_;u8 raw[40];};};
static_assert(sizeof(value_t)==48,"SCS value ABI mismatch");
struct named_value_t{cstr name;u32 index;u32 _padding;value_t value;}; static_assert(sizeof(named_value_t)==64,"SCS named value ABI mismatch");
struct configuration_t{cstr id;const named_value_t*attributes;};
typedef void(*log_fn)(s32,cstr); typedef void(*channel_cb)(cstr,u32,const value_t*,ctx_t); typedef result_t(*register_channel_fn)(cstr,u32,u32,u32,channel_cb,ctx_t);
typedef result_t(*unregister_channel_fn)(cstr,u32,u32); typedef void(*event_cb)(u32,const void*,ctx_t); typedef result_t(*register_event_fn)(u32,event_cb,ctx_t); typedef result_t(*unregister_event_fn)(u32);
struct sdk_init_v100{cstr game_name; cstr game_id; u32 game_version;u32 _padding;log_fn log;}; static_assert(sizeof(sdk_init_v100)==32,"init ABI");
struct telemetry_init_v100{sdk_init_v100 common;register_event_fn register_for_event;unregister_event_fn unregister_from_event;register_channel_fn register_for_channel;unregister_channel_fn unregister_from_channel;};static_assert(sizeof(telemetry_init_v100)==64,"telemetry ABI");
struct sockaddr_in_min{short family;u16 port;u32 addr;char zero[8];};

static AtsTelemetryPacket g{}; static f32 wears[5]{}; static f32 susp[16]{},wheelvel[16]{}; static u8 ground[16]{}; static u32 registered_wheels=0; static register_channel_fn g_reg_channel=nullptr; static SOCKET_T sock=~SOCKET_T(0); static sockaddr_in_min dst{}; static log_fn game_log=nullptr; static bool paused=true;
static bool streq(cstr a,cstr b){if(!a||!b)return false;while(*a&&*b){if(*a!=*b)return false;++a;++b;}return *a==*b;}
static void logmsg(s32 t,cstr s){if(game_log)game_log(t,s);} static void damage(){f32 m=0;for(int i=0;i<5;i++)if(wears[i]>m)m=wears[i];g.max_wear=m;g.damage_warning=m>=.65f;g.damage_critical=m>=.85f;}
static void update_wheels(){u32 n=g.wheel_count;if(n>16)n=16;if(!n){g.suspension_average=0;g.suspension_spread=0;g.wheel_ground_ratio=1;g.wheel_angular_velocity=0;return;}f32 sum=0,vsum=0,mn=susp[0],mx=susp[0];u32 on=0;for(u32 i=0;i<n;i++){sum+=susp[i];f32 v=wheelvel[i]<0?-wheelvel[i]:wheelvel[i];vsum+=v;if(susp[i]<mn)mn=susp[i];if(susp[i]>mx)mx=susp[i];if(ground[i])on++;}g.suspension_average=sum/n;g.suspension_spread=mx-mn;g.wheel_ground_ratio=(f32)on/n;g.wheel_angular_velocity=vsum/n;}
static void send_packet(){if(sock==~SOCKET_T(0))return;update_wheels();++g.sequence;sendto(sock,(const char*)&g,(int)sizeof(g),0,&dst,(int)sizeof(dst));}
static void on_float(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_FLOAT)*(f32*)c=v->value_float_.value;}
static void on_bool(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_BOOL)*(u8*)c=v->value_bool_.value?1:0;}
static void on_s32(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_S32)*(s32*)c=v->value_s32_.value;}
static void on_u32(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_U32)*(u32*)c=v->value_u32_.value;}
static void on_vec(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_FVECTOR){f32*p=(f32*)c;p[0]=v->value_fvector_.x;p[1]=v->value_fvector_.y;p[2]=v->value_fvector_.z;}}
static void on_cruise(cstr,u32,const value_t*v,ctx_t){if(v&&v->type==T_FLOAT)g.cruise=v->value_float_.value>.1f;}
static void on_wear(cstr,u32,const value_t*v,ctx_t c){if(v&&v->type==T_FLOAT){*(f32*)c=v->value_float_.value;damage();}}
static result_t regi(cstr n,u32 idx,u32 t,channel_cb cb,ctx_t c){return g_reg_channel?g_reg_channel(n,idx,t,0,cb,c):GENERIC;}
static void register_wheels(u32 count){if(count>16)count=16;for(u32 i=registered_wheels;i<count;i++){regi("truck.wheel.suspension.deflection",i,T_FLOAT,on_float,&susp[i]);regi("truck.wheel.on_ground",i,T_BOOL,on_bool,&ground[i]);regi("truck.wheel.angular_velocity",i,T_FLOAT,on_float,&wheelvel[i]);}if(count>registered_wheels)registered_wheels=count;}
static void on_event(u32 e,const void*info,ctx_t){
 if(e==EVT_FRAME_END){if(!paused)send_packet();return;} if(e==EVT_PAUSED){paused=true;return;} if(e==EVT_STARTED){paused=false;send_packet();return;} if(e!=EVT_CONFIG||!info)return;
 const configuration_t*cfg=(const configuration_t*)info;if(!streq(cfg->id,"truck")||!cfg->attributes)return;for(const named_value_t*a=cfg->attributes;a->name;++a){if(a->value.type==T_FLOAT){if(streq(a->name,"fuel.capacity"))g.fuel_capacity=a->value.value_float_.value;else if(streq(a->name,"rpm.limit"))g.rpm_limit=a->value.value_float_.value;}else if(a->value.type==T_U32&&streq(a->name,"wheels.count")){g.wheel_count=a->value.value_u32_.value;register_wheels(g.wheel_count);}}
}
static result_t reg(register_channel_fn f,cstr n,u32 t,channel_cb cb,ctx_t c){result_t r=f(n,NIL,t,0,cb,c);if(r!=OK)logmsg(LOG_WARN,"ATS DualSense: telemetry channel unavailable");return r;}

extern "C" __declspec(dllexport) result_t scs_telemetry_init(const u32 version,const void*params){
 if(version!=V101&&version!=V100)return UNSUPPORTED;if(!params)return GENERIC;const telemetry_init_v100*p=(const telemetry_init_v100*)params;game_log=p->common.log;
 alignas(8)u8 wsadata[512]{};if(WSAStartup(0x0202,wsadata)!=0){logmsg(LOG_ERR,"ATS DualSense: WSAStartup failed");return GENERIC;}sock=socket(2,2,17);if(sock==~SOCKET_T(0)){WSACleanup();return GENERIC;}
 dst.family=2;dst.port=0x8f98;dst.addr=0x0100007f;g_reg_channel=p->register_for_channel;g={};g.magic=0x41545344;g.version=4;g.size=(u16)sizeof(g);g.rpm_limit=2500;g.fuel_capacity=1;
 p->register_for_event(EVT_FRAME_END,on_event,nullptr);p->register_for_event(EVT_PAUSED,on_event,nullptr);p->register_for_event(EVT_STARTED,on_event,nullptr);p->register_for_event(EVT_CONFIG,on_event,nullptr);
 reg(p->register_for_channel,"truck.speed",T_FLOAT,on_float,&g.speed_mps);reg(p->register_for_channel,"truck.engine.rpm",T_FLOAT,on_float,&g.rpm);
 reg(p->register_for_channel,"truck.effective.throttle",T_FLOAT,on_float,&g.throttle);reg(p->register_for_channel,"truck.effective.brake",T_FLOAT,on_float,&g.brake);reg(p->register_for_channel,"truck.effective.clutch",T_FLOAT,on_float,&g.clutch);reg(p->register_for_channel,"truck.effective.steering",T_FLOAT,on_float,&g.steering);
 reg(p->register_for_channel,"truck.fuel.amount",T_FLOAT,on_float,&g.fuel);reg(p->register_for_channel,"truck.engine.gear",T_S32,on_s32,&g.gear);reg(p->register_for_channel,"truck.brake.retarder",T_U32,on_u32,&g.retarder_level);
 reg(p->register_for_channel,"truck.brake.air.pressure",T_FLOAT,on_float,&g.brake_air_pressure);reg(p->register_for_channel,"truck.brake.temperature",T_FLOAT,on_float,&g.brake_temperature);reg(p->register_for_channel,"truck.oil.pressure",T_FLOAT,on_float,&g.oil_pressure);reg(p->register_for_channel,"truck.water.temperature",T_FLOAT,on_float,&g.water_temperature);reg(p->register_for_channel,"truck.battery.voltage",T_FLOAT,on_float,&g.battery_voltage);reg(p->register_for_channel,"truck.navigation.speed.limit",T_FLOAT,on_float,&g.nav_speed_limit);
 reg(p->register_for_channel,"truck.local.acceleration.linear",T_FVECTOR,on_vec,&g.accel_x);reg(p->register_for_channel,"truck.cabin.velocity.angular",T_FVECTOR,on_vec,&g.cabin_angvel_x);reg(p->register_for_channel,"truck.cabin.acceleration.angular",T_FVECTOR,on_vec,&g.cabin_angacc_x);
 #define RB(name,field) reg(p->register_for_channel,name,T_BOOL,on_bool,&g.field)
 RB("truck.engine.enabled",engine_enabled);RB("truck.electric.enabled",electric_enabled);RB("truck.brake.parking",parking_brake);RB("truck.lblinker",left_blinker);RB("truck.rblinker",right_blinker);RB("truck.light.lblinker",left_blinker_light);RB("truck.light.rblinker",right_blinker_light);RB("truck.hazard.warning",hazards);RB("truck.light.parking",parking_lights);RB("truck.light.beam.low",low_beam);RB("truck.light.beam.high",high_beam);RB("truck.light.beacon",beacon);RB("truck.light.brake",brake_light);RB("truck.light.reverse",reverse_light);RB("truck.wipers",wipers);RB("truck.fuel.warning",fuel_warning);RB("truck.brake.air.pressure.warning",air_warning);RB("truck.brake.air.pressure.emergency",air_emergency);RB("truck.oil.pressure.warning",oil_warning);RB("truck.water.temperature.warning",water_warning);RB("truck.battery.voltage.warning",battery_warning);RB("truck.brake.motor",engine_brake);RB("truck.differential_lock",differential_lock);
 #undef RB
 reg(p->register_for_channel,"truck.cruise_control",T_FLOAT,on_cruise,nullptr);
 reg(p->register_for_channel,"truck.wear.engine",T_FLOAT,on_wear,&wears[0]);reg(p->register_for_channel,"truck.wear.transmission",T_FLOAT,on_wear,&wears[1]);reg(p->register_for_channel,"truck.wear.cabin",T_FLOAT,on_wear,&wears[2]);reg(p->register_for_channel,"truck.wear.chassis",T_FLOAT,on_wear,&wears[3]);reg(p->register_for_channel,"truck.wear.wheels",T_FLOAT,on_wear,&wears[4]);
 logmsg(LOG_MSG,"ATS DualSense Bridge v0.7: immersive telemetry initialized");return OK;
}
extern "C" __declspec(dllexport) void scs_telemetry_shutdown(void){if(sock!=~SOCKET_T(0)){closesocket(sock);sock=~SOCKET_T(0);}WSACleanup();logmsg(LOG_MSG,"ATS DualSense Bridge: telemetry shutdown");game_log=nullptr;}
