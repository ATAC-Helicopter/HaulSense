#include "../ats-dualsense/daemon/src/route_reader.hpp"
#include <cassert>
#include <chrono>
#include <cstring>
#include <fstream>
#include <limits>
#include <thread>
#include <unistd.h>
#include <sys/stat.h>
using namespace std::chrono_literals;
int main(){
 char path[]="/tmp/haulsense-route-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);
 std::string bytes(96000,'\0');uint64_t uid=0xffffffffffffffffULL;float distance=20,time=5;
 std::memcpy(bytes.data(),&uid,8);std::memcpy(bytes.data()+8,&distance,4);std::memcpy(bytes.data()+12,&time,4);
 auto write=[&]{std::ofstream f(path,std::ios::binary|std::ios::trunc);f.write(bytes.data(),bytes.size());};write();
 auto wait=[&](RouteReader& reader,const char* needle){for(int i=0;i<100;i++){if(reader.snapshot(true).find(needle)!=std::string::npos)return;std::this_thread::sleep_for(30ms);}assert(false);};
 {RouteReader reader(path);wait(reader,"\"available\":true");assert(reader.snapshot(true).find("\"fresh\":false")!=std::string::npos);
 distance=10;std::memcpy(bytes.data()+8,&distance,4);write();wait(reader,"\"fresh\":true");assert(reader.snapshot(true).find("ffffffffffffffff")!=std::string::npos);assert(reader.snapshot(false).find("\"node_uids\":[]")!=std::string::npos);
 std::this_thread::sleep_for(2600ms);assert(reader.snapshot(true).find("\"fresh\":false")!=std::string::npos);
 distance=std::numeric_limits<float>::quiet_NaN();std::memcpy(bytes.data()+8,&distance,4);write();wait(reader,"\"available\":false");}
 unlink(path);assert(mkfifo(path,0600)==0);{RouteReader reader(path);std::this_thread::sleep_for(50ms);assert(reader.snapshot(true).find("\"available\":false")!=std::string::npos);}unlink(path);
}
