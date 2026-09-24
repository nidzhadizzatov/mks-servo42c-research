#include "mks_servo42c/logger.hpp"

#include <iomanip>

namespace mks_servo42c {

Logger::Logger() = default;

Logger::~Logger() {
    close();
}

bool Logger::open(const std::string& filename) {
    close();
    file_.open(filename, std::ios::out | std::ios::trunc);
    if (!file_.is_open()) return false;
    start_time_ = std::chrono::steady_clock::now();
    write_header("timestamp_ms,error_deg,avg_error_deg,shaft_status,collision");
    return true;
}

void Logger::close() {
    if (file_.is_open()) {
        file_.flush();
        file_.close();
    }
}

void Logger::write_header(const std::string& header) {
    if (file_.is_open()) {
        file_ << header << "\n";
    }
}

void Logger::log(double error_deg,
                 double avg_error_deg,
                 uint8_t shaft_status,
                 int collision_flag) {
    if (!file_.is_open()) return;

    auto now = std::chrono::steady_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - start_time_).count();

    file_ << ms << ","
          << std::fixed << std::setprecision(4) << error_deg << ","
          << std::fixed << std::setprecision(4) << avg_error_deg << ","
          << static_cast<int>(shaft_status) << ","
          << collision_flag << "\n";
}

}  // namespace mks_servo42c