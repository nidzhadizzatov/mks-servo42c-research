#include "mks_servo42c/serial_port.hpp"
#include "mks_servo42c/servo.hpp"
#include <iostream>
#include <thread>
#include <chrono>
int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        return 1;
    }

    mks_servo42c::SerialPort port;
    if (!port.open(argv[1], 38400)) {
        std::cerr << "Failed to open " << argv[1] << "\n";
        return 1;
    }

    mks_servo42c::Servo servo(port);

    auto error = servo.read_angle_error();
    if (error) {
        std::cout << "Angle error: " << *error << " deg\n";
    } else {
        std::cout << "Failed to read angle error\n";
    }

    auto status = servo.read_shaft_status();
    if (status) {
        std::cout << "Shaft status: " << static_cast<int>(*status)
                  << " (" << (*status == 1 ? "blocked" : "unblocked") << ")\n";
    } else {
        std::cout << "Failed to read shaft status\n";
    }

    auto enc = servo.read_encoder();
    if (enc) {
        double deg = (*enc / 65535.0) * 360.0;
        std::cout << "Encoder: " << *enc << " (~" << deg << " deg)\n";
    }

    std::vector<uint8_t> resp;
// Отправим команду 0x33 вручную и посмотрим сырой ответ
{
    std::vector<uint8_t> frame = {0xE0, 0x33, 0x13};  // checksum = 0xE0+0x33 = 0x113 → 0x13
    port.write(frame.data(), frame.size());
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    uint8_t buf[16] = {};
    size_t n = port.read(buf, sizeof(buf), 200);
    std::cout << "Raw pulse_count response (" << n << " bytes): ";
    for (size_t i = 0; i < n; ++i) printf("%02X ", buf[i]);
    std::cout << "\n";
}

    port.close();
    return 0;
}