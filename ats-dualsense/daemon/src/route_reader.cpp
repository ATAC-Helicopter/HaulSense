#include "route_reader.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

RouteReader::RouteReader(std::string path):path_(std::move(path)),thread_([this]{run();}){}
RouteReader::~RouteReader(){stop_=true;wake_.notify_all();if(thread_.joinable())thread_.join();}
void RouteReader::run(){
    // Provider ABI: 6000 little-endian records of uint64 UID, float distance,
    // float travel time. No writer heartbeat is present in that ABI.
    constexpr size_t bytes=6000*16;
    std::array<unsigned char,bytes> buffer{},check{},previous{};
    bool have_previous=false,observed_change=false;
    while(!stop_){
        bool valid=false;std::string nodes="[]";
        const int fd=::open(path_.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
        if(fd>=0){
            struct stat info{};
            if(!fstat(fd,&info)&&S_ISREG(info.st_mode)&&info.st_uid==geteuid()&&info.st_size==(off_t)bytes&&
                pread(fd,buffer.data(),bytes,0)==(ssize_t)bytes&&pread(fd,check.data(),bytes,0)==(ssize_t)bytes&&buffer==check){
                std::ostringstream json;json<<"[";bool first=true;valid=true;
                for(size_t at=0;at<bytes;at+=16){
                    uint64_t uid=0;float distance=0,time=0;
                    std::memcpy(&uid,buffer.data()+at,8);std::memcpy(&distance,buffer.data()+at+8,4);std::memcpy(&time,buffer.data()+at+12,4);
                    if(!uid)break;
                    if(!std::isfinite(distance)||!std::isfinite(time)||distance<0||time<0||distance>1e8||time>1e8){valid=false;break;}
                    if(!first)json<<",";
                    first=false;json<<"\""<<std::hex<<uid<<"\"";
                }
                json<<"]";nodes=json.str();valid=valid&&!first;
            }
            ::close(fd);
        }
        {
            std::lock_guard lock(mutex_);
            available_=valid;
            if(valid){
                if(have_previous&&buffer!=previous){observed_change=true;changed_at_=std::chrono::steady_clock::now();}
                nodes_=std::move(nodes);previous=buffer;have_previous=true;changed_=observed_change;
            }else{nodes_="[]";have_previous=false;observed_change=false;changed_=false;}
        }
        std::unique_lock lock(mutex_);
        wake_.wait_for(lock,std::chrono::milliseconds(500),[this]{return stop_.load();});
    }
}
std::string RouteReader::snapshot(bool active) const{
    std::lock_guard lock(mutex_);
    const auto age=changed_?std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-changed_at_).count():-1;
    const bool fresh=available_&&changed_&&age>=0&&age<2500&&active;
    return std::string("{\"source\":\"ets2la\",\"available\":")+(available_?"true":"false")+",\"fresh\":"+(fresh?"true":"false")+
        ",\"age_ms\":"+std::to_string(age)+",\"freshness\":\"content-change-only\",\"node_uids\":"+(fresh?nodes_:"[]")+"}";
}
