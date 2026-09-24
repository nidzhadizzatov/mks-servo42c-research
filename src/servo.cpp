#include "mks_servo42c/servo.hpp"
#include "mks_servo42c/protocol.hpp"

#include <chrono>
#include <thread>
#include <cmath>

namespace mks_servo42c {

Servo::Servo(SerialPort& port, uint8_t address)
    : port_(port), address_(address) {}

bool Servo::send_and_receive(uint8_t command,
                             const std::vector<uint8_t>& data,
                             std::vector<uint8_t>& response,
                             size_t expected_response_size) {
    auto frame = build_frame(address_, command, data);
    port_.flush_input();
    if (port_.write(frame.data(), frame.size()) != frame.size()) {
        return false;
    }

    std::this_thread::sleep_for(std::chrono::milliseconds(20));

    response.resize(expected_response_size);
    size_t n = port_.read(response.data(), response.size(), 200);
    response.resize(n);
    return n >= 2;  // минимум: адрес + 1 байт данных
}

bool Servo::enable(bool on) {
    std::vector<uint8_t> resp;
    return send_and_receive(cmd::ENABLE_DRIVER, {static_cast<uint8_t>(on ? 1 : 0)}, resp, 3);
}

bool Servo::run(int speed, Direction dir) {
    speed = std::max(0, std::min(127, speed));
    uint8_t s = static_cast<uint8_t>(speed);
    if (dir == Direction::CCW) s |= 0x80;
    std::vector<uint8_t> resp;
    return send_and_receive(cmd::RUN_MOTOR, {s}, resp, 3);
}

bool Servo::stop() {
    std::vector<uint8_t> resp;
    return send_and_receive(cmd::STOP_MOTOR, {}, resp, 3);
}

bool Servo::move(int speed, Direction dir, uint16_t pulses) {
    speed = std::max(0, std::min(127, speed));
    uint8_t s = static_cast<uint8_t>(speed);
    if (dir == Direction::CCW) s |= 0x80;

    std::vector<uint8_t> data = {
        s,
        static_cast<uint8_t>((pulses >> 8) & 0xFF),
        static_cast<uint8_t>(pulses & 0xFF)
    };
    std::vector<uint8_t> resp;
    return send_and_receive(cmd::MOVE_MOTOR, data, resp, 3);
}

std::optional<double> Servo::read_angle_error() {
    std::vector<uint8_t> resp;
    if (!send_and_receive(cmd::READ_ANGLE_ERROR, {}, resp, 3)) return std::nullopt;
    if (resp.size() < 3) return std::nullopt;

    int16_t raw = static_cast<int16_t>((resp[1] << 8) | resp[2]);
    return (raw / 65536.0) * 360.0;
}

std::optional<uint8_t> Servo::read_shaft_status() {
    std::vector<uint8_t> resp;
    if (!send_and_receive(cmd::READ_SHAFT_STATUS, {}, resp, 2)) return std::nullopt;
    if (resp.size() < 2) return std::nullopt;
    return resp[1];
}

std::optional<uint16_t> Servo::read_encoder() {
    std::vector<uint8_t> resp;
    if (!send_and_receive(cmd::READ_ENCODER, {}, resp, 3)) return std::nullopt;
    if (resp.size() < 3) return std::nullopt;
    return static_cast<uint16_t>((resp[1] << 8) | resp[2]);
}

std::optional<int32_t> Servo::read_pulse_count() {
    std::vector<uint8_t> resp;
    if (!send_and_receive(cmd::READ_PULSE_COUNT, {}, resp, 5)) return std::nullopt;
    if (resp.size() < 5) return std::nullopt;
    int32_t v = (static_cast<int32_t>(resp[1]) << 24) |
                (static_cast<int32_t>(resp[2]) << 16) |
                (static_cast<int32_t>(resp[3]) << 8)  |
                (static_cast<int32_t>(resp[4]));
    return v;
}

}  // namespace mks_servo42c