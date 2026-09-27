#include "player_leds.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <unistd.h>

namespace fs = std::filesystem;

bool PlayerLeds::discover(const std::string& hidraw_path){
    previous_=0xff;
    available_=false; writable_=false; group_.clear(); paths_.fill({});
    std::map<std::string,std::array<std::string,5>> groups;
    std::regex re(R"((.*):white:player-?([1-5])$)");
    std::error_code ec;
    if(!fs::exists("/sys/class/leds",ec)) return false;
    for(const auto& e: fs::directory_iterator("/sys/class/leds",ec)){
        std::smatch m; const std::string name=e.path().filename().string();
        if(!std::regex_match(name,m,re)) continue;
        if(!hidraw_path.empty()){
            auto controller=fs::canonical(fs::path("/sys/class/hidraw")/fs::path(hidraw_path).filename()/"device",ec);
            if(ec)continue;
            auto led_device=fs::canonical(e.path()/"device",ec);
            if(ec || controller!=led_device)continue;
        }
        int idx=std::stoi(m[2].str())-1;
        groups[m[1].str()][idx]=(e.path()/"brightness").string();
    }
    for(auto& [g,p]:groups){
        bool full=true; for(auto& s:p) if(s.empty()) full=false;
        if(!full) continue;
        group_=g; paths_=p; available_=true;
        writable_=true;
        for(size_t i=0;i<paths_.size();++i){
            if(::access(paths_[i].c_str(),W_OK)!=0) writable_=false;
            std::ifstream max_file(fs::path(paths_[i]).parent_path()/"max_brightness");
            int value=1;if(max_file>>value && value>0)maximum_[i]=value;
        }
        return true;
    }
    return false;
}

bool PlayerLeds::write_one(size_t idx,bool on){
    if(idx>=paths_.size()||paths_[idx].empty()) return false;
    int brightness=on?maximum_[idx]:0;
    std::ofstream f(paths_[idx]); if(!f) return false;
    f << brightness << "\n"; return (bool)f;
}

bool PlayerLeds::set_mask(uint8_t mask){
    if(!available_) return false;
    bool ok=true;
    // HID bit 0 corresponds to player1, bit 4 to player5.
    for(size_t i=0;i<5;i++) if(previous_==0xff || ((mask^previous_)&(1u<<i))) ok = write_one(i,(mask&(1u<<i))!=0) && ok;
    if(ok)previous_=mask;
    writable_=ok; return ok;
}
