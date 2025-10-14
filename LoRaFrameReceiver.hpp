#pragma once

#include <WString.h>
#include <stdint.h>

struct LoRaFrameReceiver {
    virtual void onFrame(const uint8_t* buffer, size_t size) = 0;
    virtual void onFrame(const String& str)                  = 0;
};
