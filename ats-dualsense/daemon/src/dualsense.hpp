#pragma once
#include <cstdint>
#include <string>

class DualSense {
public:
    ~DualSense();
    bool open_first();
    bool alive() const;
    void close_device();
    bool connected() const { return fd_ >= 0; }
    bool bluetooth() const { return bluetooth_; }
    const std::string& path() const { return path_; }

    bool apply(uint8_t r, uint8_t g, uint8_t b, uint8_t player_led_mask,
               uint8_t left_position, uint8_t left_strength,
               uint8_t right_position, uint8_t right_strength,
               uint8_t motor_right, uint8_t motor_left, bool control_player_leds = true);
    bool neutral(bool control_player_leds = true);

private:
    bool send_report(const uint8_t *common47);
    static uint32_t crc32_update(uint32_t crc, const uint8_t* data, size_t len);
    int fd_ = -1;
    bool bluetooth_ = false;
    uint8_t seq_ = 0;
    std::string path_;
};
