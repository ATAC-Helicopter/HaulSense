#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>

class PlayerLeds {
public:
    bool discover(const std::string& hidraw_path = {});
    bool available() const { return available_; }
    bool writable() const { return writable_; }
    const std::string& group() const { return group_; }
    bool set_mask(uint8_t mask);
    bool off() { return set_mask(0); }
    const std::array<std::string,5>& paths() const { return paths_; }
private:
    bool write_one(size_t idx, bool on);
    std::array<std::string,5> paths_{};
    bool available_ = false;
    bool writable_ = false;
    std::string group_;
    std::array<int,5> maximum_{{1,1,1,1,1}};
    uint8_t previous_ = 0xff;
};
