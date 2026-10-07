#include "../ats-dualsense/daemon/src/protocol.hpp"
#include <cstdio>
#include <cstring>
#include "../ats-dualsense/daemon/src/legacy_protocol.hpp"
int main(int argc,char**argv){
    if(argc>1&&std::strcmp(argv[1],"--legacy")==0){LegacyTelemetryPacket old;old.sequence=1;old.electric_enabled=old.engine_enabled=1;old.speed_mps=18;old.left_blinker=old.left_blinker_light=1;std::fwrite(&old,1,sizeof(old),stdout);return 0;}
    AtsTelemetryPacket t;t.sequence=1;t.electric_enabled=t.engine_enabled=1;t.speed_mps=18;t.rpm=1400;t.gear=t.displayed_gear=8;
    t.available[0]=~0ull;t.available[1]=(1ull<<(CHANNEL_COUNT-64))-1;
    t.left_blinker=t.left_blinker_light=1;t.low_beam=1;
    t.placement_available=1;t.game=1;t.world_x=12345.125;t.world_y=42;t.world_z=-6789.5;t.heading=.75f;
    if(argc>1&&std::strncmp(argv[1],"--job-",6)==0){
        t.job_active=1;t.job_sequence=1;t.odometer=1000;t.fuel=200;t.cargo_mass=12000;
        std::strcpy(t.cargo,"Report fixture");std::strcpy(t.origin,"Bakersfield");std::strcpy(t.destination,"Los Angeles");
        if(std::strcmp(argv[1],"--job-step")==0){t.sequence=2;t.world_x+=50;t.odometer+=1;t.fuel-=1;}
        if(std::strcmp(argv[1],"--job-finish")==0){t.sequence=3;t.world_x+=100;t.odometer+=2;t.fuel-=2;t.event_sequence=1;std::strcpy(t.last_event,"job.delivered");t.event_attributes=EVENT_MONEY|EVENT_XP|EVENT_DISTANCE|EVENT_GAME_MINUTES|EVENT_AUTOPARK;t.event_money=12345;t.event_xp=500;t.event_distance=210;t.event_game_minutes=90;t.event_autopark=0;}
    }
    if(argc>1&&std::strcmp(argv[1],"--v5")==0){t.version=5;t.size=V5_PACKET_SIZE;std::fwrite(&t,1,V5_PACKET_SIZE,stdout);return 0;}
    if(argc>1&&std::strcmp(argv[1],"--v6")==0){t.version=6;t.size=V6_PACKET_SIZE;std::fwrite(&t,1,V6_PACKET_SIZE,stdout);return 0;}
    std::fwrite(&t,1,sizeof(t),stdout);
}
