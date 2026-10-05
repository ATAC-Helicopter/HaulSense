#include "dualsense.hpp"
#include <algorithm>
#include <array>
#include <cstring>
#include <filesystem>
#include <vector>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <linux/input.h>
#include <sys/ioctl.h>
#include <unistd.h>

namespace {
constexpr uint16_t SONY=0x054c, DUALSENSE=0x0ce6, DUALSENSE_EDGE=0x0df2;
constexpr uint8_t F0_RUMBLE=1u<<0, F0_HAPTICS_SELECT=1u<<1, F0_RT=1u<<2, F0_LT=1u<<3;
constexpr uint8_t F1_LIGHTBAR=1u<<2, F1_PLAYER=1u<<4;
constexpr uint8_t TRIGGER_OFF=0x05, TRIGGER_FEEDBACK=0x21;
#pragma pack(push,1)
struct Common {
    uint8_t valid0{},valid1{};
    uint8_t motor_right{},motor_left{};
    uint8_t headphone_volume{},speaker_volume{},mic_volume{},audio_control{};
    uint8_t mic_led{},power_save{};
    uint8_t right_mode{}; uint8_t right_param[10]{};
    uint8_t left_mode{};  uint8_t left_param[10]{};
    uint32_t host_timestamp{};
    uint8_t reduce_motor_power{},audio_control2{},valid2{},haptics_flags{},reserved{};
    uint8_t lightbar_setup{},led_brightness{},player_leds{},red{},green{},blue{};
};
#pragma pack(pop)
static_assert(sizeof(Common)==47);

void encode_feedback(uint8_t *out10,uint8_t position,uint8_t strength){
    std::memset(out10,0,10); if(!strength) return;
    position=std::min<uint8_t>(position,9); strength=std::clamp<uint8_t>(strength,1,8);
    uint16_t active=0; uint32_t packed=0;
    for(int i=position;i<10;i++){ active|=uint16_t(1u<<i); packed|=uint32_t((strength-1)&7u)<<(3*i); }
    out10[0]=active&0xff; out10[1]=(active>>8)&0xff;
    out10[2]=packed&0xff; out10[3]=(packed>>8)&0xff; out10[4]=(packed>>16)&0xff; out10[5]=(packed>>24)&0xff;
}
}

DualSense::~DualSense(){ close_device(); }
void DualSense::close_device(){ if(fd_>=0)::close(fd_); fd_=-1; path_.clear(); }
bool DualSense::alive() const {hidraw_devinfo info{};return fd_>=0&&ioctl(fd_,HIDIOCGRAWINFO,&info)==0;}
bool DualSense::open_first(){
    close_device();
    std::vector<std::filesystem::path> candidates;
    std::error_code ec;
    for(const auto& e:std::filesystem::directory_iterator("/dev",ec)){
        auto n=e.path().filename().string();
        if(n.rfind("hidraw",0)==0) candidates.push_back(e.path());
    }
    std::sort(candidates.begin(),candidates.end(),[](const auto& a,const auto& b){
        auto number=[](const std::filesystem::path& p){
            const auto n=p.filename().string();
            try{return std::stoi(n.substr(6));}catch(...){return 1<<30;}
        };
        return number(a)<number(b);
    });
    for(const auto& path:candidates){
        int fd=::open(path.c_str(),O_RDWR|O_NONBLOCK); if(fd<0)continue;
        hidraw_devinfo info{}; if(ioctl(fd,HIDIOCGRAWINFO,&info)<0){::close(fd);continue;}
        if(info.vendor!=SONY||(info.product!=DUALSENSE&&info.product!=DUALSENSE_EDGE)){::close(fd);continue;}
        bluetooth_=info.bustype==BUS_BLUETOOTH;
        fd_=fd; path_=path.string(); return true;
    }
    return false;
}
uint32_t DualSense::crc32_update(uint32_t crc,const uint8_t* data,size_t len){
    crc=~crc; for(size_t i=0;i<len;i++){crc^=data[i];for(int k=0;k<8;k++)crc=(crc>>1)^(0xEDB88320u&(-(int32_t)(crc&1)));} return ~crc;
}
bool DualSense::send_report(const uint8_t* common47){
    if(fd_<0)return false;
    if(!bluetooth_){std::array<uint8_t,63>b{};b[0]=0x02;std::memcpy(b.data()+1,common47,47);return ::write(fd_,b.data(),b.size())==(ssize_t)b.size();}
    std::array<uint8_t,78>b{};b[0]=0x31;b[1]=uint8_t((seq_++&0x0f)<<4);b[2]=0x10;std::memcpy(b.data()+3,common47,47);
    uint8_t seed=0xA2;uint32_t crc=crc32_update(0,&seed,1);crc=crc32_update(crc,b.data(),74);
    b[74]=crc&0xff;b[75]=(crc>>8)&0xff;b[76]=(crc>>16)&0xff;b[77]=(crc>>24)&0xff;
    return ::write(fd_,b.data(),b.size())==(ssize_t)b.size();
}
bool DualSense::apply(uint8_t r,uint8_t g,uint8_t b,uint8_t leds,uint8_t lp,uint8_t ls,uint8_t rp,uint8_t rs,uint8_t mr,uint8_t ml,bool control_player_leds){
    Common c{};
    c.valid0=F0_RT|F0_LT|F0_RUMBLE|F0_HAPTICS_SELECT;
    c.valid1=F1_LIGHTBAR|(control_player_leds?F1_PLAYER:0);
    c.motor_right=mr; c.motor_left=ml;
    c.right_mode=rs?TRIGGER_FEEDBACK:TRIGGER_OFF; c.left_mode=ls?TRIGGER_FEEDBACK:TRIGGER_OFF;
    if(rs) encode_feedback(c.right_param,rp,rs);
    if(ls) encode_feedback(c.left_param,lp,ls);
    if(control_player_leds) c.player_leds=(leds&0x1f)|0x20; // Apply atomically, without the firmware player-ID fade.
    c.red=r; c.green=g; c.blue=b;
    return send_report(reinterpret_cast<uint8_t*>(&c));
}
bool DualSense::neutral(bool control_player_leds){ return apply(0,0,0,0,0,0,0,0,0,0,control_player_leds); }

bool DualSense::player_leds(uint8_t mask){
    Common c{};c.valid1=F1_PLAYER;c.player_leds=(mask&0x1f)|0x20;
    return send_report(reinterpret_cast<uint8_t*>(&c));
}
