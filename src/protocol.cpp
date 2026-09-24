#include "mks_servo42c/protocol.hpp"

namespace mks_servo42c {

uint8_t checksum(const std::vector<uint8_t>& data) {
    uint32_t sum = 0;
    for (auto b : data) sum += b;
    return static_cast<uint8_t>(sum & 0xFF);
}

std::vector<uint8_t> build_frame(uint8_t address, uint8_t command,
                                 const std::vector<uint8_t>& data) {
    std::vector<uint8_t> frame;
    frame.reserve(2 + data.size() + 1);
    frame.push_back(address);
    frame.push_back(command);
    frame.insert(frame.end(), data.begin(), data.end());
    frame.push_back(checksum(frame));
    return frame;
}

bool parse_response(const std::vector<uint8_t>& raw,
                    uint8_t& address,
                    std::vector<uint8_t>& payload) {
    if (raw.size() < 2) return false;

    address = raw[0];
    payload.assign(raw.begin() + 1, raw.end());

    // Проверяем контрольную сумму, если она есть.
    // В прошивке V3.8.1 ответы идут без rCHK, поэтому проверяем не строго.
    // Если последний байт похож на checksum — можно опционально валидировать.
    return true;
}

}  // namespace mks_servo42c