#pragma once

#include <cstddef>
#include <cstdint>

namespace Btp {
    enum class PacketType : uint8_t {
        Data = 0,
        Ack  = 1,
    };

    struct Constants {
        static constexpr size_t mtu             = 230;
        static constexpr size_t headerSize      = 4;
        static constexpr size_t extraHeaderSize = 4;
        static constexpr size_t maxPayload      = mtu - headerSize;

        static constexpr size_t ackTimeoutMs      = 1000UL; // 1s
        static constexpr size_t transferTimeoutMs = 10000UL; // 10s
        static constexpr size_t receiveTimeoutMs  = 5000UL; // 5s
    };
} // namespace Btp
