#include "player_leds.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <map>
#include <regex>
#include <unistd.h>

namespace fs = std::filesystem;

bool PlayerLeds::discover(){
    available_=false; writable_=false; group_.clear(); paths_.fill({});
    std::map<std::string,std::array<std::string,5>> groups;
    std::regex re(R"((.*):white:player-?([1-5])$)");
    std::error_code ec;
    if(!fs::exists("/sys/class/leds",ec)) return false;
    for(const auto& e: fs::directory_iterator("/sys/class/leds",ec)){
        std::smatch m; const std::string name=e.path().filename().string();
        if(!std::regex_match(name,m,re)) continue;
        int idx=std::stoi(m[2].str())-1;
        groups[m[1].str()][idx]=(e.path()/"brightness").string();
    }
    for(auto& [g,p]:groups){
        bool full=true; for(auto& s:p) if(s.empty()) full=false;
        if(!full) continue;
        group_=g; paths_=p; available_=true;
        writable_=true; for(auto& s:paths_) if(::access(s.c_str(),W_OK)!=0) writable_=false;
        return true;
    }
    return false;
}

bool PlayerLeds::write_one(size_t idx,bool on){
    if(idx>=paths_.size()||paths_[idx].empty()) return false;
    int brightness=1;
    if(on){
        fs::path p(paths_[idx]);
        std::ifstream mf(p.parent_path()/"max_brightness");
        int m=1; if(mf>>m && m>0) brightness=m;
    } else brightness=0;
    std::ofstream f(paths_[idx]); if(!f) return false;
    f << brightness << "\n"; return (bool)f;
}

bool PlayerLeds::set_mask(uint8_t mask){
    if(!available_) return false;
    bool ok=true;
    // HID bit 0 corresponds to player1, bit 4 to player5.
    for(size_t i=0;i<5;i++) ok = write_one(i,(mask&(1u<<i))!=0) && ok;
    writable_=ok; return ok;
}
