#pragma once

#include "BtpTransport.hpp"

#include <cstdint>
#include <span>

struct BleService {
    virtual ~BleService()                                                                       = default;
    virtual void run(uint8_t type, std::span<const uint8_t> data, Btp::BtpTransport& transport) = 0;
};
