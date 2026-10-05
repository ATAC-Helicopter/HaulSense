#include "dualsense.hpp"
#include "protocol.hpp"
#include "legacy_protocol.hpp"
#include "player_leds.hpp"
#include "dashboard.hpp"
#include <filesystem>
#include <iomanip>
#include <limits>
#include <map>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <csignal>
#include <cstring>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <netinet/in.h>
#include <poll.h>
#include <sstream>
#include <string>
#include <thread>
#include <sys/socket.h>
#include <unistd.h>

using Clock = std::chrono::steady_clock;

struct Config {
    // v0.8.0 stays quiet while cruising, but allows stronger event and brake feedback.
    float rumble_strength = 0.22f;
    float road_strength = 0.28f;
    float trigger_strength = 0.66f;
    float brake_haptic_strength = 0.38f;
    float lightbar_strength = 0.68f;
    float beacon_strength = 0.24f;
    float bump_threshold = 0.46f;
    bool overspeed_warning = false;
    bool critical_lightbar_warnings = true;
    uint8_t left_indicator_inner = 0x08;  // inner-left player LED
    uint8_t left_indicator_pair = 0x18;   // both left player LEDs
    uint8_t right_indicator_inner = 0x02; // inner-right player LED
    uint8_t right_indicator_pair = 0x03;  // both right player LEDs
    uint8_t headlight_led_mask = 0x04;    // center player LED
    int indicator_step_ms = 135;
    bool effects_enabled = true;
    bool reverse_cue = false;
    bool wiper_cue = false;
    bool trailer_cue = true;
    bool swap_indicators = false;
    bool gameplay_cue = true;
};

struct Fx { uint8_t r,g,b,leds,lp,ls,rp,rs,mr,ml; };
struct Runtime {
    bool initialized = false;
    int32_t gear = 0;
    uint32_t retarder = 0;
    bool engine_brake = false;
    bool cruise = false;
    bool differential_lock = false;
    bool high_beam = false;
    bool engine_enabled = false;
    bool parking_brake = false;
    float brake = 0;
    float susp_avg = 0;
    float last_accel_y = 0;
    Clock::time_point gear_pulse{};
    Clock::time_point brake_pulse{};
    Clock::time_point bump_pulse{};
    Clock::time_point engine_brake_pulse{};
    Clock::time_point retarder_pulse{};
    Clock::time_point cruise_pulse{};
    Clock::time_point diff_pulse{};
    Clock::time_point beam_pulse{};
    Clock::time_point engine_start_pulse{};
    Clock::time_point parking_brake_pulse{};
    Clock::time_point indicator_phase_start{};
    bool indicator_phase = false;
    uint8_t direction = 0;
    uint32_t event_sequence = 0;
    Clock::time_point gameplay_pulse{};
    bool trailer_connected = false;
    bool lift_axle = false;
    float susp_spread = 0;
    Clock::time_point trailer_pulse{}, axle_pulse{};
    float bump_energy = 0;
};

static std::atomic_bool running{true};
static void stop_handler(int){ running=false; }
static float clamp01(float v){ return std::isfinite(v)?std::clamp(v,0.0f,1.0f):0.0f; }
static float ab(float v){ return std::fabs(v); }
static uint8_t u8scale(float x){ return static_cast<uint8_t>(std::clamp((int)std::lround(x),0,255)); }

static std::string trim(std::string s){
    auto first=s.find_first_not_of(" \t\r\n"); if(first==std::string::npos)return {};
    auto last=s.find_last_not_of(" \t\r\n"); return s.substr(first,last-first+1);
}
static uint8_t parse_mask(const std::string& v,uint8_t fallback){
    try { return static_cast<uint8_t>(std::stoul(v,nullptr,0)&0x1f); } catch(...) { return fallback; }
}
static Config load_config(const std::string& explicit_path){
    Config c; std::string path=explicit_path;
    if(path.empty()){
        const char* home=std::getenv("HOME");
        const char*xdg=std::getenv("XDG_CONFIG_HOME");
        if(xdg&&*xdg)path=std::string(xdg)+"/ats-dualsense/config.conf";
        else if(home)path=std::string(home)+"/.config/ats-dualsense/config.conf";
    }
    std::ifstream f(path); if(!f) return c;
    std::string line;
    while(std::getline(f,line)){
        auto hash=line.find('#'); if(hash!=std::string::npos)line.resize(hash);
        auto eq=line.find('='); if(eq==std::string::npos)continue;
        auto k=trim(line.substr(0,eq)),v=trim(line.substr(eq+1));
        try{
            if(k=="rumble_strength")c.rumble_strength=clamp01(std::stof(v));
            else if(k=="road_strength")c.road_strength=clamp01(std::stof(v));
            else if(k=="trigger_strength")c.trigger_strength=clamp01(std::stof(v));
            else if(k=="brake_haptic_strength")c.brake_haptic_strength=clamp01(std::stof(v));
            else if(k=="lightbar_strength")c.lightbar_strength=clamp01(std::stof(v));
            else if(k=="beacon_strength")c.beacon_strength=clamp01(std::stof(v));
            else if(k=="bump_threshold")c.bump_threshold=clamp01(std::stof(v));
            else if(k=="overspeed_warning")c.overspeed_warning=(v!="0"&&v!="false"&&v!="off");
            else if(k=="critical_lightbar_warnings")c.critical_lightbar_warnings=(v!="0"&&v!="false"&&v!="off");
            else if(k=="left_indicator_inner")c.left_indicator_inner=parse_mask(v,c.left_indicator_inner);
            else if(k=="left_indicator_pair")c.left_indicator_pair=parse_mask(v,c.left_indicator_pair);
            else if(k=="right_indicator_inner")c.right_indicator_inner=parse_mask(v,c.right_indicator_inner);
            else if(k=="right_indicator_pair")c.right_indicator_pair=parse_mask(v,c.right_indicator_pair);
            else if(k=="headlight_led_mask")c.headlight_led_mask=parse_mask(v,c.headlight_led_mask);
            else if(k=="indicator_step_ms")c.indicator_step_ms=std::clamp(std::stoi(v),60,300);
            else if(k=="gameplay_cue")c.gameplay_cue=(v!="0"&&v!="false"&&v!="off");
            else if(k=="effects_enabled")c.effects_enabled=(v!="0"&&v!="false"&&v!="off");
            else if(k=="reverse_cue")c.reverse_cue=(v!="0"&&v!="false"&&v!="off");
            else if(k=="wiper_cue")c.wiper_cue=(v!="0"&&v!="false"&&v!="off");
            else if(k=="trailer_cue")c.trailer_cue=(v!="0"&&v!="false"&&v!="off");
            else if(k=="swap_indicators")c.swap_indicators=(v!="0"&&v!="false"&&v!="off");
            // Legacy sysfs_player_leds is intentionally ignored: all gameplay LED updates are atomic HID reports.
        }catch(...){ }
    }
    return c;
}

static void set_rgb(Fx&f,float r,float g,float b,const Config&c){
    f.r=u8scale(r*c.lightbar_strength); f.g=u8scale(g*c.lightbar_strength); f.b=u8scale(b*c.lightbar_strength);
}
static long age_ms(Clock::time_point now, Clock::time_point then){
    if(then.time_since_epoch().count()==0) return 999999;
    return std::chrono::duration_cast<std::chrono::milliseconds>(now-then).count();
}
static float pulse_decay(Clock::time_point now,Clock::time_point then,float duration){
    long a=age_ms(now,then); if(a<0||a>=duration)return 0; return 1.0f-(float)a/duration;
}

static void update_runtime(Runtime& rt,const AtsTelemetryPacket&t,Clock::time_point now,const Config&cfg){
    const bool combined_phase = t.left_blinker_light || t.right_blinker_light;
    const uint8_t direction = (t.left_blinker?1:0)|(t.right_blinker?2:0)|(t.hazards?3:0);
    if(!rt.initialized){
        rt.initialized=true;
        rt.gear=t.gear; rt.retarder=t.retarder_level; rt.engine_brake=t.engine_brake; rt.cruise=t.cruise;
        rt.differential_lock=t.differential_lock; rt.high_beam=t.high_beam; rt.engine_enabled=t.engine_enabled;
        rt.parking_brake=t.parking_brake; rt.brake=t.brake; rt.susp_avg=t.suspension_average; rt.last_accel_y=t.accel.y;
        rt.event_sequence=t.event_sequence; rt.indicator_phase=combined_phase; rt.direction=direction; rt.trailer_connected=t.trailer_connected; rt.lift_axle=t.lift_axle; rt.susp_spread=t.suspension_spread;
        if(direction) rt.indicator_phase_start=now;
        return;
    }
    if(t.gear!=rt.gear && (t.gear!=0 || rt.gear!=0)) rt.gear_pulse=now;
    if(t.brake-rt.brake>0.34f) rt.brake_pulse=now;
    if(t.engine_brake!=rt.engine_brake && t.engine_brake) rt.engine_brake_pulse=now;
    if(t.retarder_level!=rt.retarder) rt.retarder_pulse=now;
    if(t.cruise!=rt.cruise) rt.cruise_pulse=now;
    if(t.differential_lock!=rt.differential_lock) rt.diff_pulse=now;
    if(t.high_beam!=rt.high_beam) rt.beam_pulse=now;
    if(t.engine_enabled && !rt.engine_enabled) rt.engine_start_pulse=now;
    if(t.parking_brake!=rt.parking_brake) rt.parking_brake_pulse=now;
    if((combined_phase && !rt.indicator_phase) || direction!=rt.direction) rt.indicator_phase_start=now;
    if(t.event_sequence!=rt.event_sequence){rt.gameplay_pulse=now;rt.event_sequence=t.event_sequence;}
    if(t.trailer_connected!=rt.trailer_connected)rt.trailer_pulse=now;
    if(t.lift_axle!=rt.lift_axle)rt.axle_pulse=now;

    // Road feedback only reacts to transients, not steady speed/RPM/throttle.
    float susp_step=ab(t.suspension_average-rt.susp_avg);
    float accel_step=ab(t.accel.y-rt.last_accel_y);
    float airborne=std::max(0.0f,1.0f-t.wheel_ground_ratio);
    float raw_bump=clamp01(susp_step*24.0f + ab(t.suspension_spread-rt.susp_spread)*8.0f + accel_step*0.045f + airborne*0.80f);
    if(ab(t.speed_mps)>2.0f && raw_bump>cfg.bump_threshold && age_ms(now,rt.bump_pulse)>70){rt.bump_energy=raw_bump;rt.bump_pulse=now;}
    else rt.bump_energy*=0.68f;

    rt.gear=t.gear; rt.retarder=t.retarder_level; rt.engine_brake=t.engine_brake; rt.cruise=t.cruise;
    rt.differential_lock=t.differential_lock; rt.high_beam=t.high_beam; rt.engine_enabled=t.engine_enabled;
    rt.parking_brake=t.parking_brake; rt.brake=t.brake; rt.susp_avg=t.suspension_average; rt.last_accel_y=t.accel.y;
    rt.indicator_phase=combined_phase; rt.direction=direction; rt.trailer_connected=t.trailer_connected; rt.lift_axle=t.lift_axle; rt.susp_spread=t.suspension_spread;
}

static Fx effects(const AtsTelemetryPacket&t,uint64_t ms,Clock::time_point now,Runtime&rt,const Config&cfg,bool mirrored_player_leds=false){
    Fx f{0,0,0,0,0,0,0,0,0,0};
    if(!t.electric_enabled || t.paused || !cfg.effects_enabled) {rt={};return f;}

    update_runtime(rt,t,now,cfg);
    const float rpm=t.rpm_limit>100?clamp01(t.rpm/t.rpm_limit):0;
    const float speed=ab(t.speed_mps);
    const bool lamp_clock=channel_available(t,CHANNEL_left_blinker_light)||channel_available(t,CHANNEL_right_blinker_light);
    const bool combined_blink_phase=lamp_clock?(t.left_blinker_light||t.right_blinker_light):(age_ms(now,rt.indicator_phase_start)%700<350);
    const bool hazard_requested=t.hazards || (t.left_blinker && t.right_blinker);
    // Direction comes from the logical SCS channels. Physical lamp channels are used only as the flash clock.
    const bool left_requested=hazard_requested || (t.left_blinker && !t.right_blinker);
    const bool right_requested=hazard_requested || (t.right_blinker && !t.left_blinker);

    // LIGHTBAR: persistent states are intentionally boring and stable.
    if(t.engine_enabled) set_rgb(f,4+4*rpm,12+4*rpm,38+8*rpm,cfg);
    else set_rgb(f,2,3,6,cfg);
    if(t.parking_lights) set_rgb(f,8,16,34,cfg);
    if(t.low_beam) set_rgb(f,18,34,68,cfg);
    if(t.high_beam) set_rgb(f,105,142,198,cfg);

    // Brief acknowledgements, never permanent state ownership.
    float q=0;
    if((q=pulse_decay(now,rt.engine_start_pulse,420))>0) set_rgb(f,8,48*q,90*q,cfg);
    if((q=pulse_decay(now,rt.cruise_pulse,520))>0) set_rgb(f,0,70*q,92*q,cfg);
    if((q=pulse_decay(now,rt.diff_pulse,520))>0) set_rgb(f,65*q,18*q,90*q,cfg);
    if((q=pulse_decay(now,rt.beam_pulse,300))>0 && t.high_beam) set_rgb(f,145*q,180*q,240*q,cfg);
    if((q=pulse_decay(now,rt.parking_brake_pulse,360))>0 && t.parking_brake) set_rgb(f,105*q,20*q,8*q,cfg);

    if(t.reverse_light||t.gear<0) set_rgb(f,175,190,212,cfg);

    // PLAYER LEDs: center = truck lights, outer pairs = directional sequential indicators.
    const bool truck_lights_on=t.parking_lights||t.low_beam||t.high_beam;
    if(truck_lights_on && !hazard_requested && (mirrored_player_leds || (!left_requested && !right_requested))) f.leds|=cfg.headlight_led_mask & 0x04;
    auto sequential_mask=[&](bool requested,uint8_t inner,uint8_t pair,uint8_t side)->uint8_t{
        if(!requested || !combined_blink_phase || (mirrored_player_leds && !hazard_requested)) return 0;
        long elapsed=age_ms(now,rt.indicator_phase_start); if(!lamp_clock)elapsed%=700;
        return (elapsed<cfg.indicator_step_ms?inner:pair) & side;
    };
    f.leds|=sequential_mask(left_requested,cfg.swap_indicators?cfg.right_indicator_inner:cfg.left_indicator_inner,cfg.swap_indicators?cfg.right_indicator_pair:cfg.left_indicator_pair,cfg.swap_indicators?0x03:0x18);
    f.leds|=sequential_mask(right_requested,cfg.swap_indicators?cfg.left_indicator_inner:cfg.right_indicator_inner,cfg.swap_indicators?cfg.left_indicator_pair:cfg.right_indicator_pair,cfg.swap_indicators?0x18:0x03);

    // Hazards own the RGB bar; normal indicators never touch it.
    if(hazard_requested){
        if(combined_blink_phase) set_rgb(f,218,58,0,cfg);
        else set_rgb(f,6,2,0,cfg);
    }

    // Beacon is deliberately distinct from hazards: slow, dim warm-gold breathing.
    if(t.beacon && !hazard_requested && !t.reverse_light){
        float wave=0.15f+0.25f*(0.5f+0.5f*std::sin(ms*0.0021));
        float k=cfg.beacon_strength*wave;
        set_rgb(f,160*k,72*k,3*k,cfg);
    }

    // Overspeed is opt-in and intentionally not red.
    if(cfg.overspeed_warning && t.nav_speed_limit>2.0f && speed>t.nav_speed_limit+4.2f && !hazard_requested && !t.reverse_light){
        float p=0.15f+0.12f*(0.5f+0.5f*std::sin(ms*0.0025)); set_rgb(f,48*p,12*p,72*p,cfg);
    }

    // Warning colors are sparse. Normal wear never flashes red.
    const bool amber_tick=(ms%7000)<180;
    const bool warn_tick=(ms%1500)<160;
    const bool critical_tick=(ms%720)<220;
    if(t.fuel_warning && amber_tick && !hazard_requested) set_rgb(f,145,46,0,cfg);
    if(t.air_warning && warn_tick && !t.air_emergency) set_rgb(f,175,58,0,cfg);
    if(t.battery_warning && t.engine_enabled && warn_tick) set_rgb(f,145,62,0,cfg);
    if(cfg.critical_lightbar_warnings){
        if((t.oil_warning||t.water_warning) && t.engine_enabled && warn_tick) set_rgb(f,180,5,0,cfg);
        if(t.air_emergency){ if(critical_tick)set_rgb(f,220,0,0,cfg); else set_rgb(f,12,0,0,cfg); }
        if(t.damage_critical){ if(critical_tick)set_rgb(f,205,0,0,cfg); else set_rgb(f,12,0,0,cfg); }
    }

    if(cfg.gameplay_cue && (q=pulse_decay(now,rt.gameplay_pulse,550))>0 && !hazard_requested){
        if(std::strcmp(t.last_event,"job.delivered")==0)set_rgb(f,35*q,125*q,55*q,cfg);
        else if(std::strcmp(t.last_event,"player.fined")==0)set_rgb(f,150*q,55*q,5*q,cfg);
    }

    // L2 brake: progressive resistance plus separate vibration texture below.
    if(t.brake>0.12f && cfg.trigger_strength>0){
        float b=clamp01((t.brake-0.12f)/0.88f);
        f.lp=(uint8_t)std::clamp(8-(int)std::lround(b*3.0f),5,8);
        f.ls=(uint8_t)std::clamp((int)std::lround((1.0f+3.4f*b)*cfg.trigger_strength),1,4);
    }
    if(t.air_warning && cfg.trigger_strength>0){f.lp=std::min<uint8_t>(f.lp?f.lp:7,7);f.ls=std::max<uint8_t>(f.ls,2);}
    if(t.air_emergency && cfg.trigger_strength>0){f.lp=5;f.ls=std::max<uint8_t>(f.ls,4);}

    // R2 only gives a tiny full-throttle cue.
    float demand=clamp01(t.throttle*0.75f+rpm*0.25f);
    if(t.engine_enabled && t.throttle>0.92f && demand>0.88f && cfg.trigger_strength>0.20f){f.rp=9;f.rs=1;}

    // RUMBLE/HAPTIC LAYER: mostly event-driven, with a restrained brake texture under genuinely hard braking.
    float low=0,high=0,e=0;
    if((e=pulse_decay(now,rt.engine_start_pulse,190))>0){low+=0.08f*e;high+=0.025f*e;}
    if((e=pulse_decay(now,rt.gear_pulse,115))>0){low+=0.32f*e;high+=0.075f*e;}
    if((e=pulse_decay(now,rt.brake_pulse,105))>0){high+=0.075f*e;}
    if((e=pulse_decay(now,rt.bump_pulse,125))>0){high+=0.65f*rt.bump_energy*e*cfg.road_strength;low+=0.14f*rt.bump_energy*e*cfg.road_strength;}
    if((e=pulse_decay(now,rt.engine_brake_pulse,180))>0){low+=0.10f*e;}
    if((e=pulse_decay(now,rt.retarder_pulse,145))>0){low+=0.07f*e;}
    if((e=pulse_decay(now,rt.parking_brake_pulse,110))>0){low+=0.045f*e;}

    // Brake texture: only at higher pedal pressure and while moving. Deceleration strengthens it.
    if(t.brake>0.52f && speed>4.0f && cfg.brake_haptic_strength>0){
        float b=clamp01((t.brake-0.52f)/0.48f);
        float decel=clamp01(ab(t.accel.z)/5.0f);
        bool phase=((ms/48)%2)==0; // ~10 Hz tactile pulse, not a constant buzz.
        if(phase){
            high+=(0.045f+0.10f*b+0.05f*decel)*cfg.brake_haptic_strength;
            low+=(0.018f+0.035f*b)*cfg.brake_haptic_strength;
        }
    }

    if(cfg.gameplay_cue && (e=pulse_decay(now,rt.gameplay_pulse,170))>0){low+=.13f*e;high+=.04f*e;}
    if(cfg.trailer_cue && (e=pulse_decay(now,rt.trailer_pulse,220))>0) {low+=.12f*e;high+=.04f*e;}
    if((e=pulse_decay(now,rt.axle_pulse,150))>0)low+=.06f*e;
    if(cfg.reverse_cue && t.gear<0 && speed>.3f && speed<4 && ms%1200<90)low+=.05f;
    if(cfg.wiper_cue && t.wipers && speed>2 && ms%1400<60)high+=.025f;
    if(t.parking_brake && speed>2.0f && ((ms/650)%2)==0){low+=0.055f;high+=0.035f;}
    if(t.air_warning && warn_tick) low+=0.08f;
    if(t.damage_critical && critical_tick){low+=0.10f;high+=0.07f;}

    low*=cfg.rumble_strength; high*=cfg.rumble_strength;
    f.ml=u8scale(clamp01(low)*255.0f);
    f.mr=u8scale(clamp01(high)*255.0f);
    return f;
}

static int led_test(DualSense& ds){
    std::cout<<"Player LED atomic instant-HID diagnostic.\n";
    const uint8_t masks[]={0x10,0x08,0x04,0x02,0x01,0x00};
    for(uint8_t m:masks){
        std::cout<<"mask 0x"<<std::hex<<(int)m<<std::dec<<"\n";
        if(!ds.apply(0,0,22,m,0,0,0,0,0,0,true)){std::cerr<<"LED report failed.\n";ds.neutral();return 2;}
        std::this_thread::sleep_for(std::chrono::milliseconds(900));
    }
    return ds.neutral()?0:2;
}

static void print_diagnostics(DualSense& ds, PlayerLeds& leds){
    std::cout<<"HaulSense diagnostics\n";
    if(ds.connected()) std::cout<<"Controller: "<<ds.path()<<" ("<<(ds.bluetooth()?"Bluetooth":"USB")<<")\n";
    else std::cout<<"Controller: not currently connected/writable\n";
    if(ds.connected()){
        std::cout<<"Hardware info: 0x"<<std::hex<<ds.hardware_version()<<std::dec<<"\n";
        const auto layout=ds.player_led_layout();
        std::cout<<"Player LED layout: "<<(layout==DualSense::PlayerLedLayout::Mirrored?"mirrored pairs; turn indicators use HUD only":layout==DualSense::PlayerLedLayout::Independent?"independent LEDs":"unknown; physical qualification required")<<"\n";
    }
    if(leds.discover(ds.path())){
        std::cout<<"Player LED group: "<<leds.group()<<"\n";
        std::cout<<"Player LED sysfs: "<<(leds.writable()?"writable":"found but not writable; run scripts/install-udev.sh")<<"\n";
        int n=1; for(const auto&p:leds.paths()) std::cout<<"  player"<<n++<<": "<<p<<"\n";
    } else std::cout<<"Player LED sysfs: not found\n";
}

static bool valid_packet(const AtsTelemetryPacket& t){
    if(t.magic!=0x41545344 || t.version!=5 || t.size!=sizeof(t) || t.paused>1 || t.wheel_count>64)return false;
    auto finite=[](float v){return std::isfinite(v)&&std::fabs(v)<1e8f;};
#define TELE_FLOAT(n,c) if(!finite(t.n))return false;
#define TELE_BOOL(n,c) if(t.n>1)return false;
#define TELE_U32(n,c)
#define TELE_S32(n,c)
#define TELE_VEC(n,c) if(!finite(t.n.x)||!finite(t.n.y)||!finite(t.n.z))return false;
#include "channels.inc"
#undef TELE_FLOAT
#undef TELE_BOOL
#undef TELE_U32
#undef TELE_S32
#undef TELE_VEC
    for(unsigned i=0;i<16;++i)if(!finite(t.wheel_suspension[i])||!finite(t.wheel_velocity[i]))return false;
    const float extra[]={t.rpm_limit,t.fuel_capacity,t.adblue_capacity,t.suspension_average,t.suspension_spread,t.wheel_ground_ratio,t.wheel_angular_velocity,t.max_wear,t.cargo_mass};
    for(float v:extra)if(!finite(v))return false;
    return t.last_event[47]==0&&t.truck_name[63]==0&&t.truck_brand[31]==0&&t.cargo[63]==0&&t.origin[47]==0&&t.destination[47]==0;
}
static std::string json_text(const char* text){
    std::string s="\"";
    for(const unsigned char*p=(const unsigned char*)text;*p;++p){
        if(*p=='"'||*p=='\\'){s+='\\';s+=*p;}
        else if(*p<32){const char*hex="0123456789abcdef";s+="\\u00";s+=hex[*p>>4];s+=hex[*p&15];}
        else s+=*p;
    }return s+'"';
}
static std::string config_json(const Config& c){
    std::ostringstream s;s<<std::boolalpha<<"{";
#define CF(n) s<<"\"" #n "\":"<<c.n<<",";
    CF(rumble_strength) CF(road_strength) CF(trigger_strength) CF(brake_haptic_strength)
    CF(lightbar_strength) CF(beacon_strength) CF(bump_threshold) CF(indicator_step_ms)
    CF(overspeed_warning) CF(critical_lightbar_warnings) CF(reverse_cue) CF(wiper_cue) CF(trailer_cue) CF(gameplay_cue) CF(swap_indicators)
#undef CF
    s<<"\"effects_enabled\":"<<c.effects_enabled<<"}";return s.str();
}
static std::string snapshot(const AtsTelemetryPacket&t,bool active,bool demo,const DualSense&ds,const PlayerLeds&,const Fx& fx,const Config&cfg,long age,uint64_t rejected,unsigned protocol=5){
    std::ostringstream s;s<<std::boolalpha<<std::setprecision(6);
    s<<"{\"name\":\"HaulSense\",\"version\":\"0.8.0\",\"active\":"<<active<<",\"demo\":"<<demo<<",\"paused\":"<<(bool)t.paused
      <<",\"source_protocol\":"<<protocol<<",\"availability_known\":"<<(protocol==5)<<",\"controller\":"<<ds.connected()<<",\"transport\":"<<json_text(ds.connected()?(ds.bluetooth()?"Bluetooth":"USB"):"Disconnected")
      <<",\"leds_available\":"<<ds.connected()<<",\"hardware_version\":"<<ds.hardware_version()
      <<",\"player_led_layout\":"<<json_text(ds.player_led_layout()==DualSense::PlayerLedLayout::Mirrored?"mirrored":ds.player_led_layout()==DualSense::PlayerLedLayout::Independent?"independent":"unknown")
      <<",\"age_ms\":"<<std::max(0l,age)<<",\"rejected\":"<<rejected<<",\"sequence\":"<<t.sequence
      <<",\"config\":"<<config_json(cfg)<<",\"fx\":{\"rgb\":["<<(int)fx.r<<","<<(int)fx.g<<","<<(int)fx.b<<"],\"leds\":"<<(int)fx.leds
      <<",\"brake\":"<<(int)fx.ls<<",\"throttle\":"<<(int)fx.rs<<",\"low\":"<<(int)fx.ml<<",\"high\":"<<(int)fx.mr<<"},\"telemetry\":{";
#define TELE_FLOAT(n,c) s<<"\"" #n "\":";if(channel_available(t,CHANNEL_##n))s<<t.n;else s<<"null";s<<",";
#define TELE_BOOL(n,c) s<<"\"" #n "\":";if(channel_available(t,CHANNEL_##n))s<<(bool)t.n;else s<<"null";s<<",";
#define TELE_U32(n,c) TELE_FLOAT(n,c)
#define TELE_S32(n,c) TELE_FLOAT(n,c)
#define TELE_VEC(n,c) s<<"\"" #n "\":";if(channel_available(t,CHANNEL_##n))s<<"["<<t.n.x<<","<<t.n.y<<","<<t.n.z<<"]";else s<<"null";s<<",";
#include "channels.inc"
#undef TELE_FLOAT
#undef TELE_BOOL
#undef TELE_U32
#undef TELE_S32
#undef TELE_VEC
    s<<"\"rpm_limit\":"<<t.rpm_limit<<",\"fuel_capacity\":"<<t.fuel_capacity<<",\"adblue_capacity\":"<<t.adblue_capacity<<",\"max_wear\":"<<t.max_wear
     <<",\"last_event\":"<<json_text(t.last_event)<<",\"event_sequence\":"<<t.event_sequence<<",\"cargo_mass\":"<<t.cargo_mass<<",\"truck_name\":"<<json_text(t.truck_name)<<",\"truck_brand\":"<<json_text(t.truck_brand)<<",\"cargo\":"<<json_text(t.cargo)
     <<",\"origin\":"<<json_text(t.origin)<<",\"destination\":"<<json_text(t.destination)<<",\"wheels\":[";
    for(unsigned i=0;i<std::min(static_cast<unsigned>(t.wheel_count),16u);++i){if(i)s<<",";s<<"{\"suspension\":";if(t.wheel_available[i]&1)s<<t.wheel_suspension[i];else s<<"null";
        s<<",\"velocity\":";if(t.wheel_available[i]&4)s<<t.wheel_velocity[i];else s<<"null";s<<",\"ground\":";if(t.wheel_available[i]&2)s<<(bool)t.wheel_ground[i];else s<<"null";s<<"}";}
    s<<"]}}";return s.str();
}
static bool save_settings(const std::string&body,const std::string&path,Config&config){
    Config candidate=config;std::istringstream lines(body);std::string line;std::map<std::string,std::string> entries;
    while(std::getline(lines,line,'&')){
        auto eq=line.find('=');if(eq==std::string::npos)return false;
        auto k=line.substr(0,eq),v=line.substr(eq+1);bool matched=false;
        try{
#define NUM(n,lo,hi) if(k==#n){size_t end=0;float val=std::stof(v,&end);if(end!=v.size()||!std::isfinite(val)||val<lo||val>hi)return false;candidate.n=val;matched=true;}
            NUM(rumble_strength,0,1) NUM(road_strength,0,1) NUM(trigger_strength,0,1) NUM(brake_haptic_strength,0,1)
            NUM(lightbar_strength,0,1) NUM(beacon_strength,0,1) NUM(bump_threshold,0,1) NUM(indicator_step_ms,60,300)
#undef NUM
#define BOOL(n) if(k==#n){if(v!="true"&&v!="false")return false;candidate.n=v=="true";matched=true;}
            BOOL(effects_enabled) BOOL(overspeed_warning) BOOL(critical_lightbar_warnings) BOOL(reverse_cue) BOOL(wiper_cue) BOOL(trailer_cue) BOOL(gameplay_cue) BOOL(swap_indicators)
#undef BOOL
        }catch(...){return false;}
        if(!matched||entries.contains(k))return false;
        entries[k]=v;
    }
    if(entries.empty()||path.empty())return false;
    std::error_code ec;std::filesystem::create_directories(std::filesystem::path(path).parent_path(),ec);if(ec)return false;
    auto temp=path+".tmp-"+std::to_string(getpid());
    std::ofstream out(temp,std::ios::trunc);if(!out)return false;
    std::ifstream in(path);
    while(std::getline(in,line)){auto eq=line.find('=');if(eq==std::string::npos||!entries.contains(trim(line.substr(0,eq))))out<<line<<"\n";}
    out<<"\n# HaulSense dashboard settings\n";for(auto&[k,v]:entries)out<<k<<"="<<v<<"\n";
    out.flush();if(!out){out.close();std::filesystem::remove(temp,ec);return false;}out.close();
    std::filesystem::rename(temp,path,ec);if(ec){std::filesystem::remove(temp,ec);return false;}config=candidate;return true;
}
static void demo_packet(AtsTelemetryPacket&t,uint64_t seq){
    float x=.5f+.25f*std::sin(seq*.014f);t={};t.sequence=seq;
    t.available[0]=~0ull;t.available[1]=(1ull<<(CHANNEL_COUNT-64))-1;
    t.electric_enabled=t.engine_enabled=1;t.rpm=1100+600*x;t.rpm_limit=2500;t.throttle=t.input_throttle=x;
    t.brake=t.input_brake=seq%500>420?.65f:0;t.speed_mps=27*x;t.fuel=255;t.fuel_capacity=400;
    t.fuel_consumption=.36f;t.fuel_range=708;t.nav_distance=183400;t.nav_time=8900;t.nav_speed_limit=25;
    t.left_blinker=seq%600<200;t.right_blinker=seq%600>=200&&seq%600<400;t.hazards=seq%600>=400;
    t.left_blinker_light=(t.left_blinker||t.hazards)&&seq%35<18;t.right_blinker_light=(t.right_blinker||t.hazards)&&seq%35<18;
    t.low_beam=1;t.gear=t.displayed_gear=8;t.wheel_count=6;t.wheel_ground_ratio=1;t.brake_air_pressure=116;t.brake_temperature=83;
    t.oil_pressure=52;t.oil_temperature=91;t.water_temperature=87;t.battery_voltage=28;t.odometer=28471;t.trailer_connected=1;t.cargo_mass=18500;
    t.wear_engine=.03f;t.wear_wheels=.07f;t.adblue_capacity=60;t.adblue=42;
    for(unsigned i=0;i<6;++i){t.wheel_available[i]=7;t.wheel_ground[i]=1;t.wheel_suspension[i]=.018f+std::sin(seq*.1f+i)*.002f;t.wheel_velocity[i]=24;}
    std::strcpy(t.truck_brand,"Kenworth");std::strcpy(t.truck_name,"W900");std::strcpy(t.cargo,"Industrial equipment");std::strcpy(t.origin,"Flagstaff");std::strcpy(t.destination,"Albuquerque");
}
int main(int argc,char**argv){
    bool mock=false,test_leds=false,diagnostics=false,telemetry_debug=false,no_controller=false,no_ui=false;std::string config_path;
    unsigned short telemetry_port=39055,dashboard_port=39056;
    for(int i=1;i<argc;++i){
        std::string a=argv[i];
        if(a=="--mock"){mock=true;no_controller=true;}
        else if(a=="--mock-hardware"){mock=true;no_controller=false;}
        else if(a=="--no-controller")no_controller=true;
        else if(a=="--no-ui")no_ui=true;
        else if(a=="--led-test")test_leds=true;
        else if(a=="--diagnostics")diagnostics=true;
        else if(a=="--telemetry-debug")telemetry_debug=true;
        else if((a=="--telemetry-port"||a=="--dashboard-port")&&i+1<argc){
            try{size_t end=0;std::string value=argv[++i];int port=std::stoi(value,&end);if(end!=value.size()||port<1024||port>65535)throw std::invalid_argument("port");if(a=="--telemetry-port")telemetry_port=port;else dashboard_port=port;}catch(...){std::cerr<<"Invalid port\n";return 2;}
        }
        else if(a=="--config"&&i+1<argc)config_path=argv[++i];
        else if(a=="--version"){std::cout<<"HaulSense 0.8.0\n";return 0;}
        else if(a=="--help"){std::cout<<"HaulSense 0.8.0\nDashboard: http://127.0.0.1:39056\n  --mock (UI demo, no controller writes)\n  --mock-hardware (demo effects on controller)\n  --no-controller\n  --no-ui\n  --led-test\n  --diagnostics\n  --telemetry-debug\n  --version\n  --config PATH\n";return 0;}
        else {std::cerr<<"Unknown option: "<<a<<"\n";return 2;}
    }
    if(config_path.empty()){const char*home=std::getenv("HOME");const char*xdg=std::getenv("XDG_CONFIG_HOME");if(xdg&&*xdg)config_path=std::string(xdg)+"/ats-dualsense/config.conf";else if(home)config_path=std::string(home)+"/.config/ats-dualsense/config.conf";}
    std::cout.setf(std::ios::unitbuf);std::cerr.setf(std::ios::unitbuf);
    Config cfg=load_config(config_path);std::signal(SIGINT,stop_handler);std::signal(SIGTERM,stop_handler);
    DualSense ds;PlayerLeds player_leds;
    if(!no_controller){ds.open_first();if(ds.connected())player_leds.discover(ds.path());}
    if(diagnostics){print_diagnostics(ds,player_leds);return 0;}
    if(test_leds){if(!ds.connected()){std::cerr<<"Connect the DualSense first.\n";return 2;}return led_test(ds);}
    int s=socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0);if(s<0){perror("socket");return 3;}
    sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(telemetry_port);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(s,(sockaddr*)&a,sizeof(a))<0){perror("UDP bind (is another bridge running?)");close(s);return 4;}
    Dashboard dashboard;
    if(!no_ui&&!dashboard.open(dashboard_port))std::cerr<<"Dashboard port busy; feedback remains active.\n";
    std::cout<<"HaulSense 0.8.0 | "<<(mock?"DEMO":"SCS telemetry")<<" | http://127.0.0.1:"<<dashboard_port<<"\n";
    if(ds.connected())std::cout<<"DualSense: "<<ds.path()<<" ("<<(ds.bluetooth()?"Bluetooth":"USB")<<")\n";
    AtsTelemetryPacket t{};uint64_t seq=0,rejected=0;unsigned source_protocol=5;auto last=Clock::now();auto last_probe=Clock::now()-std::chrono::seconds(5);
    auto last_health=Clock::now();
    auto next_tick=Clock::now();bool have=false,ann=false,timed=false;
    Fx prev{255,255,255,255,255,255,255,255,255,255},current{};Runtime runtime{};uint32_t last_debug_bits=~0u;
    while(running){
        auto now=Clock::now();
        if(ds.connected()&&now-last_health>std::chrono::seconds(2)){
            last_health=now;
            if(!ds.alive()){ds.close_device();last_probe=now;}
        }
        if(!no_controller&&!ds.connected()&&now-last_probe>std::chrono::seconds(2)){
            last_probe=now;
            if(ds.open_first()){player_leds.discover(ds.path());prev={255,255,255,255,255,255,255,255,255,255};std::cout<<"DualSense connected: "<<ds.path()<<"\n";}
        }
        bool active=have&&!t.paused&&(mock||now-last<std::chrono::milliseconds(500));
        dashboard.tick([&](const std::string&route,const std::string&body,int&status){
            if(route=="config"){
                if(!save_settings(body,config_path,cfg)){status=400;return std::string("{\"error\":\"Invalid settings or config could not be saved\"}");}
                return config_json(cfg);
            }
            return snapshot(t,active,mock,ds,player_leds,active?current:Fx{},cfg,have?age_ms(now,last):0,rejected,source_protocol);
        });
        pollfd descriptors[]={{s,POLLIN,0},{dashboard.fd(),POLLIN,0}};
        int wait_ms=active||mock?std::clamp((int)std::chrono::ceil<std::chrono::milliseconds>(next_tick-now).count(),1,20):100;
        poll(descriptors,2,wait_ms);
        if(descriptors[0].revents&POLLIN){
            // Drain at most 64 datagrams and retain the newest complete valid frame.
            for(int i=0;i<64;++i){AtsTelemetryPacket in{};auto n=recv(s,&in,sizeof(in),MSG_TRUNC);
                if(n<0)break;
                if(mock)continue;
                if(n==(ssize_t)sizeof(in)&&valid_packet(in)){t=in;have=true;last=Clock::now();source_protocol=5;}
                else if(n==(ssize_t)sizeof(LegacyTelemetryPacket)){
                    LegacyTelemetryPacket old{};std::memcpy(static_cast<void*>(&old),&in,sizeof(old));
                    auto converted=convert_legacy(old);
                    if(old.magic==0x41545344&&old.version==4&&old.size==sizeof(old)&&valid_packet(converted)){t=converted;have=true;last=Clock::now();source_protocol=4;}
                    else ++rejected;
                }else ++rejected;
            }
        }
        now=Clock::now();if(now<next_tick)continue;next_tick=now+std::chrono::milliseconds(20);
        if(mock){demo_packet(t,++seq);have=true;last=now;}
        active=have&&!t.paused&&(mock||now-last<std::chrono::milliseconds(500));
        if(!active){
            current={};runtime={};ann=false;
            if(!timed){if(ds.connected())ds.neutral();prev={};timed=true;}
            continue;
        }
        if(!ann){std::cout<<"Telemetry connected.\n";ann=true;runtime={};}timed=false;
        auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        current=effects(t,ms,now,runtime,cfg,ds.player_led_layout()==DualSense::PlayerLedLayout::Mirrored);
        if(telemetry_debug){uint32_t bits=(t.left_blinker?1u:0u)|(t.right_blinker?2u:0u)|((uint32_t)current.leds<<8);
            if(bits!=last_debug_bits){std::cout<<"signals L="<<(int)t.left_blinker<<" R="<<(int)t.right_blinker<<" ledMask=0x"<<std::hex<<(int)current.leds<<std::dec<<"\n";last_debug_bits=bits;}}
        if(!ds.connected())continue;
        // LED state changes are independent from RGB/rumble/trigger updates.
        // Re-sending the player command can restart firmware animation even
        // when the requested mask is unchanged; send it only on mask changes.
        if(std::memcmp(&current,&prev,sizeof(Fx))!=0){
            if(!ds.apply(current.r,current.g,current.b,current.leds,current.lp,current.ls,current.rp,current.rs,current.mr,current.ml,current.leds!=prev.leds)){
                ds.close_device();last_probe=now;std::cout<<"Controller disconnected; waiting for reconnect.\n";
            }prev=current;
        }
    }
    if(ds.connected())ds.neutral();
    close(s);
    std::cout<<"HaulSense stopped; controller neutralized.\n";return 0;
}
