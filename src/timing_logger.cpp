#include "mks_servo42c/timing_logger.hpp"

#include <iomanip>

namespace mks_servo42c {

TimingLogger::TimingLogger() = default;

TimingLogger::~TimingLogger() {
    close();
}

bool TimingLogger::open(const std::string& filename) {
    close();
    file_.open(filename, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) return false;
    start_time_ = std::chrono::steady_clock::now();
    file_ << "timestamp_ms,error_deg,avg_deg,status,collision,speed,detection_time_ms\n";
    return true;
}

void TimingLogger::close() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void TimingLogger::log(double error_deg,
                       double avg_deg,
                       uint8_t shaft_status,
                       int collision_flag,
                       int speed,
                       double detection_time_ms) {
    if (!file_.is_open()) return;

    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_).count();

    file_ << ms << ","
          << std::fixed << std::setprecision(4) << error_deg << ","
          << std::fixed << std::setprecision(4) << avg_deg << ","
          << static_cast<int>(shaft_status) << ","
          << collision_flag << ","
          << speed << ",";

    if (detection_time_ms >= 0.0) {
        file_ << std::fixed << std::setprecision(1) << detection_time_ms;
    } else {
        file_ << "-1";
    }
    file_ << "\n";
}

}  // namespace mks_servo42c