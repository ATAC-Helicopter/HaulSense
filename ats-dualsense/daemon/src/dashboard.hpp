#pragma once
#include <array>
#include <chrono>
#include <functional>
#include <string>
#include <poll.h>

class Dashboard {
public:
    using Handler = std::function<std::string(const std::string&, const std::string&, int&)>;
    ~Dashboard();
    bool open(unsigned short port = 39056);
    int fd() const { return listener_; }
    void tick(const Handler& handler);
private:
    struct Client { int fd=-1; std::string input,output; size_t sent=0; std::chrono::steady_clock::time_point start; };
    int listener_=-1;
    unsigned short port_=39056;
    std::array<Client,8> clients_{};
    void close(Client& client);
};
