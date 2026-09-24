#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace mks_servo42c {

class SerialPort {
public:
    SerialPort();
    ~SerialPort();

    // Запрещаем копирование, разрешаем перемещение
    SerialPort(const SerialPort&) = delete;
    SerialPort& operator=(const SerialPort&) = delete;
    SerialPort(SerialPort&& other) noexcept;
    SerialPort& operator=(SerialPort&& other) noexcept;

    // Открытие/закрытие
    bool open(const std::string& port_name, int baudrate);
    void close();
    bool is_open() const;

    // Запись/чтение
    size_t write(const uint8_t* data, size_t length);
    size_t read(uint8_t* buffer, size_t max_length, int timeout_ms);

    // Очистка буферов
    void flush_input();
    void flush_output();

private:
    struct Impl;
    Impl* impl_;
};

}  // namespace mks_servo42c