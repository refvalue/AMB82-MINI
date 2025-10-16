#pragma once


#include <cstddef>
#include <cstdint>
#include <memory>

namespace Btp {
    class BtpTransport;

    class BtpTransportScheduler {
    public:
        explicit BtpTransportScheduler(BtpTransport& transport);
        BtpTransportScheduler(BtpTransportScheduler&&) noexcept;
        ~BtpTransportScheduler();
        BtpTransportScheduler& operator=(BtpTransportScheduler&&) noexcept;

        bool start(const char* deviceName) const;
        void stop() const;
        bool send(const uint8_t* data, size_t size) const;

    private:
        class impl;

        std::unique_ptr<impl> impl_;
    };
} // namespace Btp
