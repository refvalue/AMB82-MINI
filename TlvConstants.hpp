#pragma once

#include <array>
#include <cstdint>

struct TlvConstants {
    static constexpr std::array<uint8_t, 9> magic{'A', 'M', 'B', '8', '2', 'V', 'B', 'X', 0x01};
    static constexpr size_t typeLengthSize = 2;
};
