#pragma once

#include <cstddef>
#include <cstdint>

namespace Btp {
    struct Constants {
        static constexpr size_t mtu = 230;

        static constexpr uint32_t ackTimeoutMs      = 1000U;
        static constexpr uint32_t transferTimeoutMs = 10000U;
        static constexpr uint32_t receiveTimeoutMs  = 5000U;

        static constexpr uint32_t ackTimeoutUs      = ackTimeoutMs * 1000U;
        static constexpr uint32_t transferTimeoutUs = transferTimeoutMs * 1000U;
        static constexpr uint32_t receiveTimeoutUs  = receiveTimeoutMs * 1000U;
    };
} // namespace Btp
