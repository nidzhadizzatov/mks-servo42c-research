#include "mks_servo42c/serial_port.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port>\n";
        std::cerr << "  Linux example: " << argv[0] << " /dev/ttyUSB0\n";
        std::cerr << "  Windows example: " << argv[0] << " COM7\n";
        return 1;
    }

    mks_servo42c::SerialPort port;
    if (!port.open(argv[1], 38400)) {
        std::cerr << "Failed to open " << argv[1] << "\n";
        return 1;
    }

    std::cout << "Port " << argv[1] << " opened at 38400 baud\n";

    // Отправим команду чтения ошибки угла (0x39)
    // Кадр: [0xE0, 0x39, checksum]  checksum = (0xE0 + 0x39) & 0xFF = 0x19
    uint8_t cmd[] = {0xE0, 0x39, 0x19};
    port.write(cmd, sizeof(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint8_t buffer[16] = {};
    size_t n = port.read(buffer, sizeof(buffer), 200);

    std::cout << "Received " << n << " bytes: ";
    for (size_t i = 0; i < n; ++i) {
        printf("%02X ", buffer[i]);
    }
    std::cout << "\n";

    port.close();
    return 0;
}