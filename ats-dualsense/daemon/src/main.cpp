#include "dualsense.hpp"
#include "protocol.hpp"
#include "player_leds.hpp"
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
    // v0.7.1 stays quiet while cruising, but allows stronger event and brake feedback.
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
    bool sysfs_player_leds = true;
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
    float bump_energy = 0;
};

static std::atomic_bool running{true};
static void stop_handler(int){ running=false; }
static float clamp01(float v){ return std::clamp(v,0.0f,1.0f); }
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
        if(home) path=std::string(home)+"/.config/ats-dualsense/config.conf";
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
            else if(k=="sysfs_player_leds")c.sysfs_player_leds=(v!="0"&&v!="false"&&v!="off");
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
    if(!rt.initialized){
        rt.initialized=true;
        rt.gear=t.gear; rt.retarder=t.retarder_level; rt.engine_brake=t.engine_brake; rt.cruise=t.cruise;
        rt.differential_lock=t.differential_lock; rt.high_beam=t.high_beam; rt.engine_enabled=t.engine_enabled;
        rt.parking_brake=t.parking_brake; rt.brake=t.brake; rt.susp_avg=t.suspension_average; rt.last_accel_y=t.accel_y;
        rt.indicator_phase=combined_phase;
        if(combined_phase) rt.indicator_phase_start=now;
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
    if(combined_phase && !rt.indicator_phase) rt.indicator_phase_start=now;

    // Road feedback only reacts to transients, not steady speed/RPM/throttle.
    float susp_step=ab(t.suspension_average-rt.susp_avg);
    float accel_step=ab(t.accel_y-rt.last_accel_y);
    float airborne=std::max(0.0f,1.0f-t.wheel_ground_ratio);
    float raw_bump=clamp01(susp_step*24.0f + t.suspension_spread*8.0f + accel_step*0.045f + airborne*0.80f);
    if(raw_bump>cfg.bump_threshold){rt.bump_energy=raw_bump;rt.bump_pulse=now;}
    else rt.bump_energy*=0.68f;

    rt.gear=t.gear; rt.retarder=t.retarder_level; rt.engine_brake=t.engine_brake; rt.cruise=t.cruise;
    rt.differential_lock=t.differential_lock; rt.high_beam=t.high_beam; rt.engine_enabled=t.engine_enabled;
    rt.parking_brake=t.parking_brake; rt.brake=t.brake; rt.susp_avg=t.suspension_average; rt.last_accel_y=t.accel_y;
    rt.indicator_phase=combined_phase;
}

static Fx effects(const AtsTelemetryPacket&t,uint64_t ms,Clock::time_point now,Runtime&rt,const Config&cfg){
    Fx f{0,0,0,0,0,0,0,0,0,0};
    if(!t.electric_enabled) return f;

    update_runtime(rt,t,now,cfg);
    const float rpm=t.rpm_limit>100?clamp01(t.rpm/t.rpm_limit):0;
    const float speed=ab(t.speed_mps);
    const bool combined_blink_phase=t.left_blinker_light||t.right_blinker_light;
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
    if(truck_lights_on) f.leds|=cfg.headlight_led_mask;
    auto sequential_mask=[&](bool requested,uint8_t inner,uint8_t pair)->uint8_t{
        if(!requested || !combined_blink_phase) return 0;
        long elapsed=age_ms(now,rt.indicator_phase_start);
        return elapsed<cfg.indicator_step_ms?inner:pair;
    };
    f.leds|=sequential_mask(left_requested,cfg.left_indicator_inner,cfg.left_indicator_pair);
    f.leds|=sequential_mask(right_requested,cfg.right_indicator_inner,cfg.right_indicator_pair);

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

    // L2 brake: progressive resistance plus separate vibration texture below.
    if(t.brake>0.12f){
        float b=clamp01((t.brake-0.12f)/0.88f);
        f.lp=(uint8_t)std::clamp(8-(int)std::lround(b*3.0f),5,8);
        f.ls=(uint8_t)std::clamp((int)std::lround((1.0f+3.4f*b)*cfg.trigger_strength),1,4);
    }
    if(t.air_warning){f.lp=std::min<uint8_t>(f.lp?f.lp:7,7);f.ls=std::max<uint8_t>(f.ls,2);}
    if(t.air_emergency){f.lp=5;f.ls=std::max<uint8_t>(f.ls,4);}

    // R2 only gives a tiny full-throttle cue.
    float demand=clamp01(t.throttle*0.75f+rpm*0.25f);
    if(t.engine_enabled && t.throttle>0.92f && demand>0.88f && cfg.trigger_strength>0.20f){f.rp=9;f.rs=1;}

    // RUMBLE/HAPTIC LAYER: mostly event-driven, with a restrained brake texture under genuinely hard braking.
    float low=0,high=0,e=0;
    if((e=pulse_decay(now,rt.engine_start_pulse,190))>0){low+=0.08f*e;high+=0.025f*e;}
    if((e=pulse_decay(now,rt.gear_pulse,115))>0){low+=0.14f*e;high+=0.035f*e;}
    if((e=pulse_decay(now,rt.brake_pulse,105))>0){high+=0.075f*e;}
    if((e=pulse_decay(now,rt.bump_pulse,125))>0){high+=0.24f*rt.bump_energy*e*cfg.road_strength;low+=0.055f*rt.bump_energy*e*cfg.road_strength;}
    if((e=pulse_decay(now,rt.engine_brake_pulse,180))>0){low+=0.10f*e;}
    if((e=pulse_decay(now,rt.retarder_pulse,145))>0){low+=0.07f*e;}
    if((e=pulse_decay(now,rt.parking_brake_pulse,110))>0){low+=0.045f*e;}

    // Brake texture: only at higher pedal pressure and while moving. Deceleration strengthens it.
    if(t.brake>0.52f && speed>4.0f && cfg.brake_haptic_strength>0){
        float b=clamp01((t.brake-0.52f)/0.48f);
        float decel=clamp01(ab(t.accel_z)/5.0f);
        bool phase=((ms/48)%2)==0; // ~10 Hz tactile pulse, not a constant buzz.
        if(phase){
            high+=(0.045f+0.10f*b+0.05f*decel)*cfg.brake_haptic_strength;
            low+=(0.018f+0.035f*b)*cfg.brake_haptic_strength;
        }
    }

    if(t.parking_brake && speed>2.0f && ((ms/650)%2)==0){low+=0.055f;high+=0.035f;}
    if(t.air_warning && warn_tick) low+=0.08f;
    if(t.damage_critical && critical_tick){low+=0.10f;high+=0.07f;}

    low*=cfg.rumble_strength; high*=cfg.rumble_strength;
    f.ml=u8scale(clamp01(low)*120.0f);
    f.mr=u8scale(clamp01(high)*138.0f);
    return f;
}

static int led_test(DualSense& ds, PlayerLeds& leds, bool prefer_sysfs){
    if(prefer_sysfs && leds.discover() && leds.writable()){
        std::cout<<"Player LED sysfs diagnostic: "<<leds.group()<<"\n";
        const uint8_t masks[]={0x10,0x08,0x04,0x02,0x01,0x00};
        for(uint8_t m:masks){
            std::cout<<"mask 0x"<<std::hex<<(int)m<<std::dec<<"\n";
            leds.set_mask(m);
            ds.apply(0,0,22,0,0,0,0,0,0,0,false);
            std::this_thread::sleep_for(std::chrono::milliseconds(900));
        }
        leds.off(); ds.neutral(false); return 0;
    }
    std::cout<<"Player LED raw-HID fallback diagnostic. Install the v0.6 udev rule for exact per-LED control.\n";
    const uint8_t masks[]={0x10,0x08,0x04,0x02,0x01,0x00};
    for(uint8_t m:masks){
        std::cout<<"mask 0x"<<std::hex<<(int)m<<std::dec<<"\n";
        ds.apply(0,0,22,m,0,0,0,0,0,0,true);
        std::this_thread::sleep_for(std::chrono::milliseconds(900));
    }
    ds.neutral(); return 0;
}

static void print_diagnostics(DualSense& ds, PlayerLeds& leds){
    std::cout<<"ATS DualSense Bridge diagnostics\n";
    if(ds.connected()) std::cout<<"Controller: "<<ds.path()<<" ("<<(ds.bluetooth()?"Bluetooth":"USB")<<")\n";
    else std::cout<<"Controller: not currently connected/writable\n";
    if(leds.discover()){
        std::cout<<"Player LED group: "<<leds.group()<<"\n";
        std::cout<<"Player LED sysfs: "<<(leds.writable()?"writable":"found but not writable; run scripts/install-udev.sh")<<"\n";
        int n=1; for(const auto&p:leds.paths()) std::cout<<"  player"<<n++<<": "<<p<<"\n";
    } else std::cout<<"Player LED sysfs: not found\n";
}

int main(int argc,char**argv){
    bool mock=false,test_leds=false,diagnostics=false,telemetry_debug=false; std::string config_path;
    for(int i=1;i<argc;i++){
        std::string a=argv[i];
        if(a=="--mock")mock=true;
        else if(a=="--led-test")test_leds=true;
        else if(a=="--diagnostics")diagnostics=true;
        else if(a=="--telemetry-debug")telemetry_debug=true;
        else if(a=="--config"&&i+1<argc)config_path=argv[++i];
        else if(a=="--version"){std::cout<<"ats-dualsense 0.7.1\n";return 0;}
        else if(a=="--help"){std::cout<<"ats-dualsense v0.7.1\n  --mock\n  --led-test\n  --diagnostics\n  --telemetry-debug\n  --version\n  --config PATH\n";return 0;}
    }
    std::cout.setf(std::ios::unitbuf); std::cerr.setf(std::ios::unitbuf);
    Config cfg=load_config(config_path);
    std::signal(SIGINT,stop_handler);std::signal(SIGTERM,stop_handler);
    DualSense ds; PlayerLeds player_leds;
    ds.open_first(); player_leds.discover();
    std::cout<<"ATS DualSense Bridge v0.7.1\n";
    if(ds.connected()) std::cout<<"DualSense: "<<ds.path()<<" ("<<(ds.bluetooth()?"Bluetooth":"USB")<<")\n";
    else std::cout<<"DualSense not connected yet; service will keep waiting and hot-plug automatically.\n";
    if(diagnostics){ print_diagnostics(ds,player_leds); return 0; }
    if(test_leds){
        if(!ds.connected()){std::cerr<<"Connect the DualSense first.\n";return 2;}
        return led_test(ds,player_leds,cfg.sysfs_player_leds);
    }

    int s=socket(AF_INET,SOCK_DGRAM,0);if(s<0){perror("socket");return 3;} sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons(39055);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
    if(bind(s,(sockaddr*)&a,sizeof(a))<0){perror("bind");return 4;} if(!mock)std::cout<<"Waiting for ATS telemetry on 127.0.0.1:39055...\n";
    AtsTelemetryPacket t{};uint64_t seq=0;auto last=Clock::now();auto last_probe=Clock::now()-std::chrono::seconds(5);bool have=mock,ann=false,timed=false;
    Fx prev{255,255,255,255,255,255,255,255,255,255};Runtime runtime{}; uint8_t last_sysfs_leds=0xff; uint32_t last_debug_bits=0xffffffffu;
    while(running){
        auto loop_now=Clock::now();
        if(!ds.connected() && loop_now-last_probe>std::chrono::milliseconds(900)){
            last_probe=loop_now;
            if(ds.open_first()){
                player_leds.discover(); last_sysfs_leds=0xff; prev={255,255,255,255,255,255,255,255,255,255};
                std::cout<<"DualSense connected: "<<ds.path()<<" ("<<(ds.bluetooth()?"Bluetooth":"USB")<<")\n";
                if(cfg.sysfs_player_leds && player_leds.available() && !player_leds.writable()) std::cout<<"Player LEDs found but not writable; run scripts/install-udev.sh for exact left/right control.\n";
            }
        }
        pollfd p{s,POLLIN,0};int rc=poll(&p,1,mock?30:35);
        if(rc>0&&(p.revents&POLLIN)){
            AtsTelemetryPacket in{};auto n=recv(s,&in,sizeof(in),0);
            if(n>=(ssize_t)offsetof(AtsTelemetryPacket,reserved)&&in.magic==0x41545344&&in.version==4){t=in;have=true;last=Clock::now();timed=false;if(!ann){std::cout<<"ATS telemetry connected. Immersion engine v0.7.1 active.\n";ann=true;runtime={};}}
        } else if(mock){
            double x=(seq++%240)/239.0;t.electric_enabled=t.engine_enabled=1;t.rpm=650+1750*x;t.rpm_limit=2500;t.throttle=x;t.brake=(x>0.82)?(x-0.82)*5.5:0;t.speed_mps=28*x;t.fuel=55;t.fuel_capacity=100;t.left_blinker_light=((seq/20)%2);t.left_blinker=1;t.parking_lights=1;t.wheel_count=6;t.wheel_ground_ratio=1;t.suspension_average=0.015f+(float)std::sin(seq*.31)*0.002f;t.suspension_spread=(float)ab(std::sin(seq*.23))*0.004f;t.accel_y=(float)std::sin(seq*.35)*0.25f;t.engine_brake=((seq/120)%2);
        }
        if(!mock&&have&&Clock::now()-last>std::chrono::milliseconds(750)){
            if(!timed){
                if(player_leds.writable()) player_leds.off();
                if(ds.connected()) ds.neutral(!(cfg.sysfs_player_leds&&player_leds.writable()));
                prev={255,255,255,255,255,255,255,255,255,255};last_sysfs_leds=0xff;timed=true;ann=false;runtime={};
                std::cout<<"ATS telemetry paused/lost; controller returned to neutral.\n";
            }continue;
        }
        if(!have||!ds.connected())continue;
        auto now=Clock::now();auto ms=std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        Fx f=effects(t,(uint64_t)ms,now,runtime,cfg);
        const bool exact_leds=cfg.sysfs_player_leds && player_leds.writable();
        if(telemetry_debug){
            uint32_t bits=(t.left_blinker?1u:0u)|(t.right_blinker?2u:0u)|(t.left_blinker_light?4u:0u)|(t.right_blinker_light?8u:0u)|(t.hazards?16u:0u)|((uint32_t)f.leds<<8);
            if(bits!=last_debug_bits){
                std::cout<<"signals L="<<(int)t.left_blinker<<" R="<<(int)t.right_blinker
                         <<" lampL="<<(int)t.left_blinker_light<<" lampR="<<(int)t.right_blinker_light
                         <<" hazards="<<(int)t.hazards<<" ledMask=0x"<<std::hex<<(int)f.leds<<std::dec<<"\n";
                last_debug_bits=bits;
            }
        }
        if(exact_leds && f.leds!=last_sysfs_leds){ if(player_leds.set_mask(f.leds)) last_sysfs_leds=f.leds; }
        if(std::memcmp(&f,&prev,sizeof(Fx))!=0){
            // Live directional LEDs are sysfs-only. Raw player masks are intentionally disabled in live mode
            // because some firmware/kernel combinations collapse them into non-directional patterns.
            bool ok=ds.apply(f.r,f.g,f.b,0,f.lp,f.ls,f.rp,f.rs,f.mr,f.ml,false);
            if(!ok){
                std::cout<<"DualSense disconnected; waiting for reconnect...\n"; ds.close_device(); player_leds.off(); last_probe=Clock::now();
            }
            prev=f;
        }
    }
    if(player_leds.writable()) player_leds.off();
    if(ds.connected()) ds.neutral(!(cfg.sysfs_player_leds&&player_leds.writable()));
    close(s);
    std::cout<<"DualSense effects stopped; controller neutralized.\n";
    return 0;
}
