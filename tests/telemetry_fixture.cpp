#include "../ats-dualsense/daemon/src/protocol.hpp"
#include <cstdio>
#include <cstring>
#include "../ats-dualsense/daemon/src/legacy_protocol.hpp"
int main(int argc,char**argv){
    if(argc>1&&std::strcmp(argv[1],"--legacy")==0){LegacyTelemetryPacket old;old.sequence=1;old.electric_enabled=old.engine_enabled=1;old.speed_mps=18;old.left_blinker=old.left_blinker_light=1;std::fwrite(&old,1,sizeof(old),stdout);return 0;}
    AtsTelemetryPacket t;t.sequence=1;t.electric_enabled=t.engine_enabled=1;t.speed_mps=18;t.rpm=1400;t.gear=t.displayed_gear=8;
    t.available[0]=~0ull;t.available[1]=(1ull<<(CHANNEL_COUNT-64))-1;
    t.left_blinker=t.left_blinker_light=1;t.low_beam=1;
    std::fwrite(&t,1,sizeof(t),stdout);
}
