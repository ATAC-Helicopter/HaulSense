#define main haulsense_entrypoint
#include "../ats-dualsense/daemon/src/main.cpp"
#undef main
#include <cassert>
int main(){
    Config cfg;AtsTelemetryPacket t;t.electric_enabled=t.engine_enabled=1;t.available[CHANNEL_left_blinker_light/64]|=1ull<<(CHANNEL_left_blinker_light%64);
    auto now=Clock::now();Runtime rt;t.left_blinker=1;t.left_blinker_light=t.right_blinker_light=1;
    auto f=effects(t,1000,now,rt,cfg);assert(f.leds==0x08);
    f=effects(t,1150,now+std::chrono::milliseconds(150),rt,cfg);assert(f.leds==0x18 && !(f.leds&3));
    t.left_blinker=0;t.right_blinker=1;f=effects(t,1160,now+std::chrono::milliseconds(160),rt,cfg);assert(f.leds==0x02);
    t.hazards=1;f=effects(t,1170,now+std::chrono::milliseconds(170),rt,cfg);assert(f.leds==0x0a);
    f=effects(t,1350,now+std::chrono::milliseconds(350),rt,cfg);assert(f.leds==0x1b);
    t.low_beam=1;f=effects(t,1360,now+std::chrono::milliseconds(360),rt,cfg);assert(f.leds==0x1b);
    cfg.swap_indicators=true;t.hazards=0;t.right_blinker=0;t.left_blinker=1;rt={};f=effects(t,1000,now,rt,cfg);assert(f.leds==0x02);
    cfg.swap_indicators=false;t.low_beam=0;t.left_blinker=0;t.right_blinker=1;
    rt={};t.left_blinker_light=t.right_blinker_light=0;
    f=effects(t,1000,now,rt,cfg);assert(f.leds==0);
    t.right_blinker_light=1;f=effects(t,1020,now+std::chrono::milliseconds(20),rt,cfg);assert(f.leds==2);
    f=effects(t,1200,now+std::chrono::milliseconds(200),rt,cfg);assert(f.leds==3);
    cfg.right_indicator_inner=cfg.right_indicator_pair=0x1f;rt={};
    f=effects(t,1000,now,rt,cfg);assert(f.leds==3); // custom masks cannot leak to center/opposite side
    t.paused=1;f=effects(t,1000,now,rt,cfg);assert(f.leds==0&&f.ls==0&&f.ml==0);
    t.paused=0;cfg.effects_enabled=false;f=effects(t,1000,now,rt,cfg);assert(f.leds==0);
    cfg.effects_enabled=true;cfg.trigger_strength=0;t.brake=1;t.air_emergency=1;f=effects(t,1000,now,rt,cfg);assert(f.ls==0 && f.rs==0);
    t.air_emergency=0;t.brake=0;t.speed_mps=20;t.suspension_spread=.15f;rt={};effects(t,1000,now,rt,cfg);f=effects(t,1050,now+std::chrono::milliseconds(50),rt,cfg);assert(f.mr==0&&f.ml==0);
    LegacyTelemetryPacket old;old.left_blinker=1;old.speed_mps=16;auto legacy=convert_legacy(old);assert(legacy.left_blinker&&legacy.speed_mps==16&&!channel_available(legacy,CHANNEL_fuel_range));
    assert(valid_packet(t));t.speed_mps=std::numeric_limits<float>::quiet_NaN();assert(!valid_packet(t));
    t={};t.size=0;assert(!valid_packet(t));t={};assert(valid_packet(t));
    t.available[0]=t.available[1]=0;auto json=snapshot(t,false,false,DualSense{},PlayerLeds{},Fx{},cfg,0,0);assert(json.find("\"speed_mps\":null")!=std::string::npos);
    auto temp=std::filesystem::temp_directory_path()/"haulsense-test-config.conf";
    {std::ofstream out(temp);out<<"# user config\nunknown_custom=42\nrumble_strength=0.2\n";}
    assert(save_settings("rumble_strength=0&trigger_strength=0",temp.string(),cfg));assert(cfg.rumble_strength==0);
    assert(!save_settings("rumble_strength=nan",temp.string(),cfg));assert(!save_settings("rumble_strength=0.2&evil=true",temp.string(),cfg));assert(cfg.rumble_strength==0);
    {std::ifstream in(temp);std::string all((std::istreambuf_iterator<char>(in)),{});assert(all.find("unknown_custom=42")!=std::string::npos);}
    std::filesystem::remove(temp);std::cout<<"Directional LEDs, pause, mute, trigger disable, road transients, packet validation, availability, settings: passed\n";
}
