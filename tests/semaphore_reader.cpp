#include "../ats-dualsense/daemon/src/semaphore_reader.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <thread>
#include <unistd.h>
using namespace std::chrono_literals;
int main(){char path[]="/tmp/haulsense-signals-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);std::string bytes(1920,'\0');
 auto put=[&](size_t at,auto value){std::memcpy(bytes.data()+at,&value,sizeof(value));};put(0,4.f);put(4,10.f);put(8,-4.f);put(12,int16_t(-200));put(14,int16_t(14));put(32,int32_t(1));put(36,5.f);put(40,int32_t(2));put(44,int32_t(17));
 auto write=[&]{std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(bytes.data(),bytes.size());};write();
 auto wait=[&](SemaphoreReader& r,const char* expected){for(int i=0;i<100;i++){if(r.snapshot(true).find(expected)!=std::string::npos)return;std::this_thread::sleep_for(20ms);}assert(false);};
 {SemaphoreReader reader(path);wait(reader,"\"available\":true");assert(reader.snapshot(true).find("\"fresh\":false")!=std::string::npos);put(36,4.f);write();wait(reader,"\"fresh\":true");auto s=reader.snapshot(true);assert(s.find("-102396")!=std::string::npos&&s.find("\"state\":2")!=std::string::npos);assert(reader.snapshot(false).find("\"objects\":[]")!=std::string::npos);
 std::this_thread::sleep_for(1600ms);assert(reader.snapshot(true).find("\"fresh\":false")!=std::string::npos);put(40,int32_t(99));write();wait(reader,"\"available\":false");put(40,int32_t(8));put(0,std::numeric_limits<float>::quiet_NaN());write();wait(reader,"\"available\":false");}
 unlink(path);
}
