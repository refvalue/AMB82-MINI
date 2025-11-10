#pragma once

#include "BtpTransport.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>

namespace Btp {
    class BtpTransportScheduler {
    public:
        explicit BtpTransportScheduler(BtpTransport& transport);
        BtpTransportScheduler(BtpTransportScheduler&&) noexcept;
        ~BtpTransportScheduler();
        BtpTransportScheduler& operator=(BtpTransportScheduler&&) noexcept;

        bool start(const char* deviceName) const;
        void send(const uint8_t* data, size_t size) const;
        void send(std::span<const uint8_t> data) const;
        void onDataReceived(BtpTransport::DataHandler handler) const;

    private:
        class impl;

        std::unique_ptr<impl> impl_;
    };
} // namespace Btp
