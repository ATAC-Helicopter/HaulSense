#include "semaphore_reader.hpp"
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <fcntl.h>
#include <sstream>
#include <sys/stat.h>
#include <unistd.h>
SemaphoreReader::SemaphoreReader(std::string path):path_(std::move(path)),thread_([this]{run();}){}
SemaphoreReader::~SemaphoreReader(){stop_=true;wake_.notify_all();if(thread_.joinable())thread_.join();}
void SemaphoreReader::run(){
    // Independent implementation of the published ETS2LA packed 40 x 48-byte ABI.
    std::array<unsigned char,1920> buffer{},check{},previous{};bool have=false;
    while(!stop_){
        bool valid=false;std::string objects="[]";
        int fd=open(path_.c_str(),O_RDONLY|O_NONBLOCK|O_CLOEXEC|O_NOFOLLOW);
        if(fd>=0){
            struct stat info{};
            if(!fstat(fd,&info)&&S_ISREG(info.st_mode)&&info.st_uid==geteuid()&&info.st_size==1920&&pread(fd,buffer.data(),1920,0)==1920&&pread(fd,check.data(),1920,0)==1920&&buffer==check){
                valid=true;std::ostringstream out;out<<"[";bool first=true;
                for(size_t at=0;at<1920;at+=48){
                    auto get=[&]<typename T>(size_t offset){T value;std::memcpy(&value,buffer.data()+at+offset,sizeof(T));return value;};
                    int type=get.operator()<int32_t>(32);if(!type)continue;
                    float x=get.operator()<float>(0),y=get.operator()<float>(4),z=get.operator()<float>(8),time=get.operator()<float>(36);
                    x+=get.operator()<int16_t>(12)*512.;z+=get.operator()<int16_t>(14)*512.;
                    int state=get.operator()<int32_t>(40),id=get.operator()<int32_t>(44);
                    bool known=type==1?(state==0||state==1||state==2||state==4||state==8||state==32):(type==2&&state>=0&&state<=3);
                    if(!known||!std::isfinite(x)||!std::isfinite(y)||!std::isfinite(z)||!std::isfinite(time)||std::abs(x)>=1e8||std::abs(y)>=1e8||std::abs(z)>=1e8||time<0||time>1e6){valid=false;break;}
                    if(!first)out<<",";
                    first=false;out<<"{\"position\":["<<x<<","<<y<<","<<z<<"],\"type\":"<<type<<",\"state\":"<<state<<",\"id\":"<<id<<",\"remaining_s\":"<<time<<"}";
                }
                out<<"]";objects=out.str();
            }close(fd);
        }
        {std::lock_guard lock(mutex_);available_=valid;
            if(valid){if(have&&buffer!=previous){changed_=true;changed_at_=std::chrono::steady_clock::now();}objects_=std::move(objects);previous=buffer;have=true;}
            else {objects_="[]";have=false;changed_=false;}
        }
        std::unique_lock lock(mutex_);wake_.wait_for(lock,std::chrono::milliseconds(100),[this]{return stop_.load();});
    }
}
std::string SemaphoreReader::snapshot(bool active) const{
    std::lock_guard lock(mutex_);
    auto age=changed_?std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-changed_at_).count():-1;
    bool fresh=available_&&changed_&&age>=0&&age<1500&&active;
    return std::string("{\"source\":\"ets2la\",\"available\":")+(available_?"true":"false")+",\"fresh\":"+(fresh?"true":"false")+",\"age_ms\":"+std::to_string(age)+",\"freshness\":\"content-change-only\",\"objects\":"+(fresh?objects_:"[]")+"}";
}
