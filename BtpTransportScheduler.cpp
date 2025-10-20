#include "BtpTransportScheduler.hpp"

#include "BtpTransport.hpp"
#include "ManagedTask.hpp"
#include "ObjectPool.hpp"

#if 1
#include <FreeRTOS.h>
#endif

#include <cstring>

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
    } // namespace

    class BtpTransportScheduler::impl {
    public:
        impl(BtpTransport& transport) : transport_{transport}, txQueue_{} {}

        ~impl() {
            stop();
        }

        bool start(const char* deviceName) {
            if (!transport_.begin(deviceName)) {
                Serial.print("Failed to initialize BLE device: ");
                Serial.println(deviceName);

                return false;
            }

            Serial.print("BLE device with name `");
            Serial.print(deviceName);
            Serial.println("` started.");

            txQueue_ = xQueueCreate(8, sizeof(TxMessage));
            rxTask_  = {std::bind_front(&impl::rxTaskRoutine, this)};
            txTask_ = {std::bind_front(&impl::txTaskRoutine, this)};

            return true;
        }

        void stop() {
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

        bool send(const uint8_t* data, size_t size) {
            if (txQueue_) {
                const auto msg = pool_.acquire();

                if (msg == nullptr) {
                    Serial.println("BtpTransportScheduler: Tx queue full.");

                    return false;
                }

                msg->data = std::make_unique<uint8_t[]>(size);
                msg->size = size;
                std::memcpy(msg->data.get(), data, size);

                return xQueueSend(txQueue_, msg, 0) == pdTRUE;
            }

            return false;
        }

    private:
        void rxTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped) {
            while (!checkStopped()) {
                transport_.poll();
                vTaskDelay(pdMS_TO_TICKS(50));
            }
        }

        void txTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped) {
            TxMessage* msg{};
            while (!checkStopped()) {
                if (xQueueReceive(txQueue_, &msg, portMAX_DELAY) == pdTRUE && msg) {
                    transport_.send(msg->data.get(), msg->size);
                    pool_.release(msg);
                }
            }
        }

        BtpTransport& transport_;
        QueueDefinition* txQueue_;
        TxMessagePool pool_;
        ManagedTask rxTask_;
        ManagedTask txTask_;
    };

    BtpTransportScheduler::BtpTransportScheduler(BtpTransport& transport) : impl_{std::make_unique<impl>(transport)} {}

    BtpTransportScheduler::BtpTransportScheduler(BtpTransportScheduler&&) noexcept = default;

    BtpTransportScheduler::~BtpTransportScheduler() = default;

    BtpTransportScheduler& BtpTransportScheduler::operator=(BtpTransportScheduler&&) noexcept = default;

    bool BtpTransportScheduler::start(const char* deviceName) const {
        return impl_->start(deviceName);
    }

    void BtpTransportScheduler::stop() const {
        impl_->stop();
    }

    bool BtpTransportScheduler::send(const uint8_t* data, size_t size) const {
        return impl_->send(data, size);
    }
} // namespace Btp
