#pragma once

#include "BtpTransport.hpp"
#include "CommonTypes.hpp"

#include <cstdint>

#include <WString.h>

struct cJSON;

struct BleService {
    virtual ~BleService()                                                               = default;
    virtual void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) = 0;
};
