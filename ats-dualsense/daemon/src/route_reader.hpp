#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>

// Read-only optional ETS2LA route provider. File work stays off the effect loop.
class RouteReader {
public:
    explicit RouteReader(std::string path);
    ~RouteReader();
    std::string snapshot(bool active) const;
private:
    void run();
    std::string path_;
    std::atomic<bool> stop_{false};
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool available_=false,changed_=false;
    std::string nodes_="[]";
    std::chrono::steady_clock::time_point changed_at_{};
    std::thread thread_;
};
