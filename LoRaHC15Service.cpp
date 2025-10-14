#include "LoRaHC15Service.hpp"

#if 1
#include <FreeRTOS.h>
#endif

#include <HardwareSerial.h>
#include <portmacro.h>
#include <semphr.h>
#include <string.h>
#include <wiring.h>
#include <wiring_constants.h>
#include <wiring_digital.h>

namespace {
    struct commands {
        static constexpr char setEcoMode[] = "AT+OP=2,2";
    };

    constexpr uint32_t readingTimeout = 3000;
    constexpr uint8_t currentVersion  = 0x01;
    constexpr uint8_t magicNumber[]   = {0x7A, 0x9C, 0x3F, 0x12};

    struct frameHeader {
        uint8_t version{};
        uint16_t size{};
    };

    int32_t timedRead(Stream& stream, uint32_t timeout) {
        auto startTime = millis();

        do {
            if (const auto c = stream.read(); c >= 0) {
                return c;
            }
        } while ((millis() - startTime) < timeout);
        // -1 indicates timeout
        return -1;
    }
} // namespace

LoRaHC15Service::LoRaHC15Service(HardwareSerial& serial, uint32_t keyPin, uint32_t auxPin)
    : keyPin_{keyPin}, auxPin_{auxPin}, serial_{serial}, mutex_{xSemaphoreCreateMutex()}, receiver_{} {}

LoRaHC15Service::~LoRaHC15Service() {}

void LoRaHC15Service::init() {
    // Initializes the command pin and aux pin.
    pinMode(keyPin_, OUTPUT_OPENDRAIN);
    digitalWrite(keyPin_, LOW);

    pinMode(auxPin_, OUTPUT_OPENDRAIN);
    digitalWrite(auxPin_, HIGH);

    serial_.begin(9600);
    serial_.write(commands::setEcoMode);
    serial_.readString();

    // Starts the receiving task.
    digitalWrite(keyPin_, HIGH);
    task_ = {&LoRaHC15Service::receivingRoutine, this};
}

void LoRaHC15Service::sendData(const String& str) {
    serial_.write(str.c_str(), str.length());
}

void LoRaHC15Service::sendData(const uint8_t* buffer, size_t size) {
    serial_.write(buffer, size);
}

void LoRaHC15Service::setReceiver(LoRaFrameReceiver* receiver) {
    xSemaphoreTake(mutex_, portMAX_DELAY);
    receiver_ = receiver;
    xSemaphoreGive(mutex_);
}

void LoRaHC15Service::receivingRoutine(ManagedTask::CheckStoppedHandler checkStopped, void* param) {
    auto&& self = *static_cast<LoRaHC15Service*>(param);

    String str;
    bool timeout{};
    frameHeader header;
    uint8_t signature[sizeof(magicNumber) + sizeof(frameHeader)]{};

    // Detects the leading magic number.
    while (!checkStopped()) {
        while (self.serial_.available() >= sizeof(magicNumber)
               && self.serial_.readBytes(signature, sizeof(signature)) == sizeof(signature)
               && memcmp(signature, magicNumber, sizeof(magicNumber)) == 0) {
            header.version = signature[sizeof(magicNumber)];
            header.size    = (signature[sizeof(magicNumber) + 1] << 8) + signature[sizeof(magicNumber) + 2];

            if (header.version != currentVersion || header.size == 0) {
                continue;
            }

            timeout = false;

            for (str = String{}; str.length() < header.size;) {
                if (const auto c = timedRead(self.serial_, readingTimeout); c > -1) {
                    str += static_cast<char>(c);
                    continue;
                }

                timeout = true;
                break;
            }

            const auto receiver = [&] {
                xSemaphoreTake(self.mutex_, portMAX_DELAY);
                auto result = self.receiver_;
                xSemaphoreGive(self.mutex_);
                return result;
            }();

            if (!timeout && receiver) {
                receiver->onFrame(reinterpret_cast<const uint8_t*>(str.c_str()), str.length());
                receiver->onFrame(str);
            }
        }

        vTaskDelay(1 / portTICK_PERIOD_MS);
    }
}
