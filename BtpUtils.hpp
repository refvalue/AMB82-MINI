#pragma once

#include <stddef.h>
#include <stdint.h>

namespace Btp {
    constexpr uint16_t readU16Be(const uint8_t* p) noexcept {
        return (uint16_t(p[0]) << 8) | uint16_t(p[1]);
    }

    constexpr uint32_t readU32Be(const uint8_t* p) noexcept {
        return (uint32_t(p[0]) << 24) | (uint32_t(p[1]) << 16) | (uint32_t(p[2]) << 8) | uint32_t(p[3]);
    }

    constexpr void writeU16Be(uint8_t* p, uint16_t v) noexcept {
        p[0] = (uint8_t) (v >> 8);
        p[1] = (uint8_t) (v & 0xFF);
    }

    constexpr void writeU32Be(uint8_t* p, uint32_t v) noexcept {
        p[0] = (uint8_t) (v >> 24);
        p[1] = (uint8_t) (v >> 16);
        p[2] = (uint8_t) (v >> 8);
        p[3] = (uint8_t) (v & 0xFF);
    }
} // namespace Btp
