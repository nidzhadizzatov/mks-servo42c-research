#pragma once

#include <chrono>
#include <cstdint>
#include <fstream>
#include <string>

namespace mks_servo42c {

class Logger {
public:
    Logger();
    ~Logger();

    // Открыть CSV-файл. Первая строка — заголовок.
    bool open(const std::string& filename);

    // Закрыть файл (вызывается автоматически в деструкторе).
    void close();

    // Записать одну строку данных.
    // Формат: timestamp_ms, error, avg_error, status, collision_flag
    void log(double error_deg,
             double avg_error_deg,
             uint8_t shaft_status,
             int collision_flag);

    // Записать заголовок вручную (опционально).
    void write_header(const std::string& header);

    bool is_open() const { return file_.is_open(); }

private:
    std::ofstream file_;
    std::chrono::steady_clock::time_point start_time_;
};

}  // namespace mks_servo42c