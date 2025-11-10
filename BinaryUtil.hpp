#pragma once

#include <cstdint>
#include <span>

namespace BinaryUtil {
    constexpr uint16_t readU16Be(std::span<const uint8_t> buffer) noexcept {
        return (static_cast<uint16_t>(buffer[0]) << 8) | static_cast<uint16_t>(buffer[1]);
    }

    constexpr uint32_t readU32Be(std::span<const uint8_t> buffer) noexcept {
        return (static_cast<uint32_t>(buffer[0]) << 24) | (static_cast<uint32_t>(buffer[1]) << 16)
             | (static_cast<uint32_t>(buffer[2]) << 8) | static_cast<uint32_t>(buffer[3]);
    }

    constexpr uint64_t readU64Be(std::span<const uint8_t> buffer) noexcept {
        return (static_cast<uint64_t>(buffer[0]) << 56) | (static_cast<uint64_t>(buffer[1]) << 48)
             | (static_cast<uint64_t>(buffer[2]) << 40) | (static_cast<uint64_t>(buffer[3]) << 32)
             | (static_cast<uint64_t>(buffer[4]) << 24) | (static_cast<uint64_t>(buffer[5]) << 16)
             | (static_cast<uint64_t>(buffer[6]) << 8) | static_cast<uint64_t>(buffer[7]);
    }

    constexpr void writeU16Be(std::span<uint8_t> buffer, uint16_t value) noexcept {
        buffer[0] = static_cast<uint8_t>(value >> 8);
        buffer[1] = static_cast<uint8_t>(value & 0xFF);
    }

    constexpr void writeU32Be(std::span<uint8_t> buffer, uint32_t value) noexcept {
        buffer[0] = static_cast<uint8_t>(value >> 24);
        buffer[1] = static_cast<uint8_t>(value >> 16);
        buffer[2] = static_cast<uint8_t>(value >> 8);
        buffer[3] = static_cast<uint8_t>(value & 0xFF);
    }

    constexpr void writeU64Be(std::span<uint8_t> buffer, uint64_t value) noexcept {
        buffer[0] = static_cast<uint8_t>(value >> 56);
        buffer[1] = static_cast<uint8_t>(value >> 48);
        buffer[2] = static_cast<uint8_t>(value >> 40);
        buffer[3] = static_cast<uint8_t>(value >> 32);
        buffer[4] = static_cast<uint8_t>(value >> 24);
        buffer[5] = static_cast<uint8_t>(value >> 16);
        buffer[6] = static_cast<uint8_t>(value >> 8);
        buffer[7] = static_cast<uint8_t>(value & 0xFF);
    }
} // namespace BinaryUtil
