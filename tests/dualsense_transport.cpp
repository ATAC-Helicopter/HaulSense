#include <cstdint>
#include <string>
// Inspect transport bytes without requiring or writing to a physical controller.
#define private public
#include "../ats-dualsense/daemon/src/dualsense.hpp"
#undef private
#include <array>
#include <cassert>
#include <sys/socket.h>
#include <unistd.h>

int main() {
    using Layout=DualSense::PlayerLedLayout;
    assert(DualSense::classify_player_leds(0x313,false)==Layout::Independent);
    assert(DualSense::classify_player_leds(0x414,false)==Layout::Mirrored);
    assert(DualSense::classify_player_leds(0x514,false)==Layout::Mirrored);
    assert(DualSense::classify_player_leds(0,false)==Layout::Unknown);
    assert(DualSense::classify_player_leds(0x614,false)==Layout::Unknown);
    assert(DualSense::classify_player_leds(0x514,true)==Layout::Unknown);
    for (bool bluetooth : {false, true}) {
        int sockets[2];
        assert(socketpair(AF_UNIX, SOCK_SEQPACKET, 0, sockets) == 0);
        DualSense controller;
        controller.fd_ = sockets[0];
        controller.bluetooth_ = bluetooth;
        const size_t offset = bluetooth ? 3 : 1;
        std::array<uint8_t, 78> report{};
        for (uint8_t mask : {0x08, 0x18, 0x02, 0x03, 0x0a, 0x1b, 0x04, 0x00}) {
            assert(controller.apply(1, 2, 3, mask, 0, 0, 0, 0, 0, 0));
            assert(read(sockets[1], report.data(), report.size()) == (bluetooth ? 78 : 63));
            assert(report[0] == (bluetooth ? 0x31 : 0x02));
            assert(report[offset + 1] & 0x10); // Player LED ownership.
            assert(report[offset + 43] == (mask | 0x20)); // Exact side + instant flag.
            assert(report[offset + 44] == 1 && report[offset + 45] == 2 && report[offset + 46] == 3);
        }
        assert(controller.apply(1, 2, 3, 0x18, 0, 0, 0, 0, 10, 20, false));
        assert(read(sockets[1], report.data(), report.size()) == (bluetooth ? 78 : 63));
        assert(!(report[offset+1] & 0x10)); // changing rumble/RGB must not retrigger player LEDs
        assert(report[offset+2] == 10 && report[offset+3] == 20);
        assert(report[offset+43] == 0 && report[offset+44] == 1);
        assert(controller.player_leds(0x18));
        assert(read(sockets[1], report.data(), report.size()) == (bluetooth ? 78 : 63));
        assert(report[offset] == 0 && report[offset+1] == 0x10);
        assert(report[offset+43] == 0x38 && report[offset+44] == 0);
        assert(controller.neutral());
        assert(read(sockets[1], report.data(), report.size()) > 0);
        assert(report[offset + 43] == 0x20); // Clear instantly on pause/shutdown.
        close(sockets[1]);
    }
}
