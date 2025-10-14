#pragma once

#include "LoRaFrameReceiver.hpp"

#include <WString.h>
#include <stdint.h>

struct LoRaService {
    virtual void init()                                       = 0;
    virtual void sendData(const String& str)                  = 0;
    virtual void sendData(const uint8_t* buffer, size_t size) = 0;
    virtual void setReceiver(LoRaFrameReceiver* receiver)     = 0;
};
