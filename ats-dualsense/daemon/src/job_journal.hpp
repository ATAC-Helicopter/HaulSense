#pragma once
#include "protocol.hpp"
#include <memory>
#include <string>
class JobJournal {
public:
    explicit JobJournal(std::string directory);
    ~JobJournal();
    void observe(const AtsTelemetryPacket& packet,unsigned protocol);
    std::string list() const;
    std::string report(const std::string& id) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
