#pragma once

#include <cstdint>

uint16_t crc16_cal(uint8_t *ptr, uint32_t len, uint16_t crc_init);
