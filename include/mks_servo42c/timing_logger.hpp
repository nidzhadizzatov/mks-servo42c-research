#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>

namespace mks_servo42c {

class TimingLogger {
public:
    TimingLogger();
    ~TimingLogger();

    bool open(const std::string& filename);
    void close();

    // Логировать один замер
    void log(double error_deg,
             double avg_deg,
             uint8_t shaft_status,
             int collision_flag,
             int speed,
             double detection_time_ms = -1.0);

    bool is_open() const { return file_.is_open(); }

private:
    std::ofstream file_;
    std::chrono::steady_clock::time_point start_time_;
};

}  // namespace mks_servo42c