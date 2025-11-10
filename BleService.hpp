#pragma once

#include "BtpTransport.hpp"

#include <cstdint>
#include <functional>
#include <span>

struct BleService {
    using SendHandler = std::function<void(std::span<const uint8_t> data)>;

    virtual ~BleService()                                                                  = default;
    virtual void run(uint8_t type, std::span<const uint8_t> data, SendHandler sendHandler) = 0;
};
