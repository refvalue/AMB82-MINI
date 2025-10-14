#pragma once

#include <cstdint>
#include <functional>
#include <memory>

namespace Btp {
    class BtpTransport {
    public:
        using DataCallback  = std::function<void(uint8_t*, size_t)>;
        using ErrorCallback = std::function<void(const char*)>;

        BtpTransport(const char* serviceUuid, const char* rxUuid, const char* txUuid);
        ~BtpTransport();

        bool begin(const char* deviceName) const;
        void poll() const;
        bool send(const uint8_t* data, size_t length) const;

        void setOnDataReceived(DataCallback cb) const;
        void setOnError(ErrorCallback cb) const;
        int32_t getConnID() const noexcept;

    private:
        class impl;

        std::unique_ptr<impl> impl_;
    };
} // namespace Btp
