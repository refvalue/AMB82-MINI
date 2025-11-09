#include "BtpTransportScheduler.hpp"

#include "BtpTransport.hpp"
#include "ManagedTask.hpp"
#include "MessageQueue.hpp"

#if 1
#include <FreeRTOS.h>
#endif

#include <algorithm>
#include <utility>

#include <LOGUARTClass.h>
#include <portmacro.h>
#include <queue.h>

namespace Btp {
    class BtpTransportScheduler::impl {
    public:
        impl(BtpTransport& transport) : transport_{transport}, queue_{10} {}

        ~impl() {
            transport_.onDataReceived([](uint8_t*, size_t) {});
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

            return true;
        }

        void send(const uint8_t* data, size_t size) {
            enqueueData(data, size, [this](const uint8_t* data, size_t size) { transport_.send(data, size); });
        }

        void onDataReceived(BtpTransport::DataHandler handler) {
            transport_.onDataReceived(
                [this, handler = std::move(handler)](uint8_t* data, size_t size) { enqueueData(data, size, handler); });
        }

    private:
        void enqueueData(const uint8_t* data, size_t size, BtpTransport::DataHandler handler) {
            std::shared_ptr buffer{std::make_unique<uint8_t[]>(size)};

            std::ranges::copy(data, data + size, buffer.get());

            queue_.beginInvoke(
                [handler = std::move(handler), buffer = std::move(buffer), size] { handler(buffer.get(), size); });
        }

        BtpTransport& transport_;
        MessageQueue queue_;
        BtpTransport::DataHandler onDataReceived_;
    };

    BtpTransportScheduler::BtpTransportScheduler(BtpTransport& transport) : impl_{std::make_unique<impl>(transport)} {}

    BtpTransportScheduler::BtpTransportScheduler(BtpTransportScheduler&&) noexcept = default;

    BtpTransportScheduler::~BtpTransportScheduler() = default;

    BtpTransportScheduler& BtpTransportScheduler::operator=(BtpTransportScheduler&&) noexcept = default;

    bool BtpTransportScheduler::start(const char* deviceName) const {
        return impl_->start(deviceName);
    }

    void BtpTransportScheduler::send(const uint8_t* data, size_t size) const {
        impl_->send(data, size);
    }

    void BtpTransportScheduler::onDataReceived(BtpTransport::DataHandler handler) const {
        impl_->onDataReceived(std::move(handler));
    }
} // namespace Btp
