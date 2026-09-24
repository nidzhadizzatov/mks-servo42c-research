#pragma once

#include "mks_servo42c/serial_port.hpp"
#include <cstdint>
#include <optional>

namespace mks_servo42c {

enum class Direction : uint8_t {
    CW  = 0,   // по часовой
    CCW = 1,   // против часовой
};

class Servo {
public:
    Servo(SerialPort& port, uint8_t address = 0xE0);

    // Управление
    bool enable(bool on = true);
    bool run(int speed, Direction dir);
    bool stop();
    bool move(int speed, Direction dir, uint16_t pulses);

    // Чтение
    std::optional<double>  read_angle_error();     // 0x39, в градусах
    std::optional<uint8_t> read_shaft_status();    // 0x3E, 1=blocked, 2=unblocked
    std::optional<uint16_t> read_encoder();        // 0x30
    std::optional<int32_t> read_pulse_count();     // 0x33

private:
    bool send_and_receive(uint8_t command,
                          const std::vector<uint8_t>& data,
                          std::vector<uint8_t>& response,
                          size_t expected_response_size);

    SerialPort& port_;
    uint8_t address_;
};

}  // namespace mks_servo42c