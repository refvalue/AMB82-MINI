#include "BtpTransportScheduler.hpp"

#include "ObjectPool.hpp"

#if 1
#include <FreeRTOS.h>
#endif

#include <LOGUARTClass.h>
#include <portmacro.h>
#include <queue.h>

namespace Btp {
    namespace {
        struct TxMessage {
            std::unique_ptr<uint8_t[]> data;
            size_t size{};
        };

        using TxMessagePool = ObjectPool<TxMessage, 10>;

        TxMessagePool& getPool() {
            return *ManagedTask::getTlsData<TxMessagePool>(0, []() -> void* { return new TxMessagePool; });
        }
    } // namespace

    BtpTransportScheduler::BtpTransportScheduler(Btp::BtpTransport& transport) : transport_{transport}, txQueue_{} {}

    BtpTransportScheduler::~BtpTransportScheduler() {
        stop();
    }

    bool BtpTransportScheduler::start(const char* deviceName) {
        if (!transport_.begin(deviceName)) {
            Serial.print("Failed to initialize BLE device: ");
            Serial.println(deviceName);

            return false;
        }

        txQueue_ = xQueueCreate(8, sizeof(TxMessage));
        rxTask_  = {[](ManagedTask::CheckStoppedHandler checkStopped, void* param) {
                       static_cast<BtpTransportScheduler*>(param)->rxTaskRoutine(checkStopped);
                   },
             this};

        txTask_ = {[](ManagedTask::CheckStoppedHandler checkStopped, void* param) {
                       static_cast<BtpTransportScheduler*>(param)->txTaskRoutine(checkStopped);
                   },
            this};

        return true;
    }

    void BtpTransportScheduler::stop() {
        rxTask_.requestStop();
        txTask_.requestStop();
        rxTask_.join();
        txTask_.join();

        if (txQueue_) {
            vQueueDelete(txQueue_);
            txQueue_ = nullptr;
        }

        Serial.println("BtpTransportScheduler stopped.");
    }

    bool BtpTransportScheduler::send(const uint8_t* data, size_t size) {
        if (txQueue_) {
            auto msg = getPool().acquire();

            if (msg == nullptr) {
                Serial.println("BtpTransportScheduler: Tx queue full.");

                return false;
            }

            msg->data = std::make_unique<uint8_t[]>(size);
            msg->size = size;
            memcpy(msg->data.get(), data, size);

            return xQueueSend(txQueue_, msg, 0) == pdTRUE;
        }

        return false;
    }

    void BtpTransportScheduler::rxTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped) {
        while (!checkStopped()) {
            transport_.poll();
            vTaskDelay(pdMS_TO_TICKS(50));
        }
    }

    void BtpTransportScheduler::txTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped) {
        TxMessage* msg{};

        while (!checkStopped()) {
            if (xQueueReceive(txQueue_, &msg, portMAX_DELAY) == pdTRUE && msg) {
                transport_.send(msg->data.get(), msg->size);
                getPool().release(msg);
            }
        }

        delete &getPool();
    }
} // namespace Btp
