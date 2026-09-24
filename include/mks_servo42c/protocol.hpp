#pragma once

#include <cstdint>
#include <vector>
#include <optional>

namespace mks_servo42c {

// Команды (из документации MKS SERVO42C)
namespace cmd {
    constexpr uint8_t READ_ENCODER      = 0x30;
    constexpr uint8_t READ_PULSE_COUNT  = 0x33;
    constexpr uint8_t READ_SHAFT_ANGLE  = 0x36;
    constexpr uint8_t READ_ANGLE_ERROR  = 0x39;
    constexpr uint8_t READ_EN_STATUS    = 0x3A;
    constexpr uint8_t READ_SHAFT_STATUS = 0x3E;

    constexpr uint8_t ENABLE_DRIVER     = 0xF3;
    constexpr uint8_t RUN_MOTOR         = 0xF6;
    constexpr uint8_t STOP_MOTOR        = 0xF7;
    constexpr uint8_t MOVE_MOTOR        = 0xFD;
}

// Адрес по умолчанию
constexpr uint8_t DEFAULT_ADDRESS = 0xE0;

// Контрольная сумма: сумма байтов по модулю 256
uint8_t checksum(const std::vector<uint8_t>& data);

// Формирует кадр: [addr, cmd, data..., checksum]
std::vector<uint8_t> build_frame(uint8_t address, uint8_t command,
                                 const std::vector<uint8_t>& data = {});

// Парсит ответ: возвращает payload (без адреса) и адрес.
// Возвращает false, если данные некорректны.
bool parse_response(const std::vector<uint8_t>& raw,
                    uint8_t& address,
                    std::vector<uint8_t>& payload);

}  // namespace mks_servo42c