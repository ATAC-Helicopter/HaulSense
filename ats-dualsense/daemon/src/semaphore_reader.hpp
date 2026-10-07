#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
class SemaphoreReader {
public:
    explicit SemaphoreReader(std::string path);
    ~SemaphoreReader();
    std::string snapshot(bool active) const;
private:
    void run();
    std::string path_,objects_="[]";
    std::atomic<bool> stop_{false};
    mutable std::mutex mutex_;
    std::condition_variable wake_;
    bool available_=false,changed_=false;
    std::chrono::steady_clock::time_point changed_at_{};
    std::thread thread_;
};
