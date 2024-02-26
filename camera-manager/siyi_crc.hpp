#pragma once

#include <cstdint>

namespace siyi {

std::uint16_t crc16_cal(const std::uint8_t* ptr, std::uint32_t len);

} // namespace siyi