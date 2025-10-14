#pragma once

#include "LoRaService.hpp"
#include "ManagedTask.hpp"

#include <WString.h>
#include <stdint.h>

class HardwareSerial;
struct QueueDefinition;

class LoRaHC15Service : public LoRaService {
public:
    LoRaHC15Service(HardwareSerial& serial, uint32_t keyPin, uint32_t auxPin);
    ~LoRaHC15Service();
    void init() override;
    void sendData(const String& str) override;
    void sendData(const uint8_t* buffer, size_t size) override;
    void setReceiver(LoRaFrameReceiver* receiver) override;

private:
    static void receivingRoutine(ManagedTask::CheckStoppedHandler checkStopped, void* param);

    uint32_t keyPin_;
    uint32_t auxPin_;
    HardwareSerial& serial_;

    QueueDefinition* mutex_;
    LoRaFrameReceiver* receiver_;
    ManagedTask task_;
};
