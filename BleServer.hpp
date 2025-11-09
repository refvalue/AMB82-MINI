#pragma once

#include "CommonTypes.hpp"

#include <cstdint>
#include <memory>
#include <type_traits>

#include <WString.h>

struct BleService;

class BleServer {
public:
    BleServer(const String& deviceName, const String& serviceUuid, const String& rxUuid, const String& txUuid);
    BleServer(BleServer&&) noexcept;
    ~BleServer();
    BleServer& operator=(BleServer&&) noexcept;
    void addService(int32_t type, BleService* service) const;

    template <typename Enum>
        requires std::is_enum_v<Enum>
    void addService(Enum type, BleService* service) const {
        addService(static_cast<uint8_t>(type), service);
    }

    void start() const;
    void stop() const;

private:
    class impl;

    std::unique_ptr<impl> impl_;
};
