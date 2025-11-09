#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <span>

namespace Btp {
    class BtpTransport {
    public:
        using DataHandler  = std::function<void(uint8_t*, size_t)>;
        using ErrorHandler = std::function<void(const char*)>;

        BtpTransport(const char* serviceUuid, const char* rxUuid, const char* txUuid);
        BtpTransport(BtpTransport&&) noexcept;
        ~BtpTransport();
        BtpTransport& operator=(BtpTransport&&) noexcept;

        bool begin(const char* deviceName) const;
        bool send(const uint8_t* data, size_t size) const;
        bool send(std::span<const uint8_t> data) const;

        void onDataReceived(DataHandler handler) const;
        void onError(ErrorHandler handler) const;
        int32_t getConnID() const noexcept;

    private:
        class impl;

        std::unique_ptr<impl> impl_;
    };
} // namespace Btp
