#include "mks_servo42c/serial_port.hpp"
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
        std::cerr << "Failed to open " << argv[1] << "\n";
        return 1;
    }

    // Команда 0x33 — чтение счётчика импульсов
    // Кадр: [0xE0, 0x33, checksum]  checksum = (0xE0 + 0x33) & 0xFF = 0x13
    uint8_t cmd[] = {0xE0, 0x33, 0x13};
    port.flush_input();
    port.write(cmd, sizeof(cmd));

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    uint8_t buf[32] = {};
    size_t n = port.read(buf, sizeof(buf), 300);

    std::cout << "Received " << n << " bytes: ";
    for (size_t i = 0; i < n; ++i) {
        printf("%02X ", buf[i]);
    }
    printf("\n");

    // Попробуем разные интерпретации
    if (n >= 3) {
        uint16_t u16 = (buf[1] << 8) | buf[2];
        std::cout << "As uint16 (buf[1..2]): " << u16 << "\n";
    }
    if (n >= 5) {
        int32_t i32 = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
        std::cout << "As int32  (buf[1..4]): " << i32 << "\n";
        uint32_t u32 = (buf[1] << 24) | (buf[2] << 16) | (buf[3] << 8) | buf[4];
        std::cout << "As uint32 (buf[1..4]): " << u32 << "\n";
    }
    if (n >= 7) {
        int32_t i32b = (buf[3] << 24) | (buf[4] << 16) | (buf[5] << 8) | buf[6];
        std::cout << "As int32  (buf[3..6]): " << i32b << "\n";
    }

    port.close();
    return 0;
}