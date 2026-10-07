#include "../ats-dualsense/daemon/src/job_journal.hpp"
#include "../ats-dualsense/third_party/json/json.hpp"
#include <cassert>
#include <cstring>
#include <chrono>
#include <filesystem>
#include <thread>
using Json=nlohmann::json;using namespace std::chrono_literals;
int main(){char path[]="/tmp/haulsense-jobs-XXXXXX";assert(mkdtemp(path));AtsTelemetryPacket t;t.job_active=1;t.game=1;t.job_sequence=1;t.sequence=1;t.placement_available=1;t.world_x=10;t.world_z=20;t.world_y=3;t.speed_mps=10;t.fuel=100;t.odometer=1000;t.available[0]=~0ull;t.available[1]=~0ull;std::strcpy(t.cargo,"Cargo <safe>");std::strcpy(t.origin,"Start");std::strcpy(t.destination,"Finish");
 auto delivered=[&](JobJournal& j){for(int i=0;i<100;i++){auto list=Json::parse(j.list());if(list["reports"].size())return list;std::this_thread::sleep_for(20ms);}assert(false);return Json();};
 std::string id;
 {JobJournal journal(path);journal.observe(t,7);std::this_thread::sleep_for(100ms);++t.sequence;t.world_x=30;t.odometer=1001;t.fuel=99;t.event_sequence=1;std::strcpy(t.last_event,"player.fined");t.event_attributes=EVENT_MONEY;t.event_money=200;journal.observe(t,7);
 ++t.sequence;t.world_x=50;t.odometer=1002;t.fuel=98;t.event_sequence=2;std::strcpy(t.last_event,"job.delivered");t.event_attributes=EVENT_MONEY|EVENT_XP|EVENT_DISTANCE|EVENT_GAME_MINUTES|EVENT_AUTOPARK;t.event_money=12345;t.event_xp=42;t.event_distance=12;t.event_game_minutes=90;t.event_autopark=0;journal.observe(t,7);
 auto list=delivered(journal);id=list["reports"][0]["id"];auto report=Json::parse(journal.report(id));assert(report["official"]["revenue"]==12345&&report["official"]["earned_xp"]==42);assert(report["fuel_used_l"]==2&&report["distance_km"]==2);assert(report["route"].size()==3&&report["fines"].size()==1);assert(report["outcome"]=="delivered");++t.sequence;journal.observe(t,7);std::this_thread::sleep_for(50ms);assert(Json::parse(journal.list())["reports"].size()==1);t.job_active=0;t.cargo[0]=0;++t.sequence;journal.observe(t,7);}
 {JobJournal journal(path);auto list=delivered(journal);assert(list["reports"].size()==1);assert(Json::parse(journal.report(id))["cargo"]=="Cargo <safe>");assert(journal.report("../config")=="null");}
 // An unfinished job survives a daemon restart with a visible gap.
 t.job_active=1;t.sequence=1;t.event_sequence=0;t.last_event[0]=0;t.cargo[0]='X';t.cargo[1]=0;
 {JobJournal journal(path);journal.observe(t,7);std::this_thread::sleep_for(100ms);}
 {JobJournal journal(path);std::this_thread::sleep_for(100ms);t.sequence=2;t.event_sequence=1;std::strcpy(t.last_event,"job.cancelled");journal.observe(t,7);for(int i=0;i<100&&Json::parse(journal.list())["reports"].size()<2;i++)std::this_thread::sleep_for(20ms);auto list=Json::parse(journal.list());assert(list["reports"].size()==2);auto r=Json::parse(journal.report(list["reports"][0]["id"]));assert(r["outcome"]=="cancelled"&&r["gaps"]>=1);}
 std::filesystem::remove_all(path);
}
