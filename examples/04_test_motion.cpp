#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstdio>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    mks_servo42c::SerialPort port;
    if (!port.open(argv[1], 38400)) {
        std::cerr << "Failed to open\n";
        return 1;
    }

    mks_servo42c::Servo servo(port);

    // Функция чтения pulse count с диагностикой
    auto read_pulse = [&]() -> int32_t {
        uint8_t cmd[] = {0xE0, 0x33, 0x13};
        port.flush_input();
        port.write(cmd, sizeof(cmd));
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        uint8_t buf[16] = {};
        size_t n = port.read(buf, sizeof(buf), 200);
        if (n >= 5) {
            return (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
        }
        return 0;
    };

    std::cout << "=== Before motion ===\n";
    std::cout << "Pulse count: " << read_pulse() << "\n";

    std::cout << "\n=== Enabling driver ===\n";
    servo.enable(true);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    std::cout << "=== Running CW at speed 30 for 2 seconds ===\n";
    servo.run(30, mks_servo42c::Direction::CW);
    std::this_thread::sleep_for(std::chrono::seconds(2));
    servo.stop();
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    std::cout << "\n=== After motion ===\n";
    std::cout << "Pulse count: " << read_pulse() << "\n";

    auto error = servo.read_angle_error();
    if (error) std::cout << "Angle error: " << *error << " deg\n";

    auto enc = servo.read_encoder();
    if (enc) std::cout << "Encoder: " << *enc << "\n";

    port.close();
    return 0;
}