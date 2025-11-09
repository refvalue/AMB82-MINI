#include "BleServer.hpp"

#include "BleService.hpp"
#include "BtpTransport.hpp"
#include "BtpTransportScheduler.hpp"

#include <unordered_map>

#include <LOGUARTClass.h>

class BleServer::impl {
public:
    impl(const String& deviceName, const String& serviceUuid, const String& rxUuid, const String& txUuid)
        : deviceName_{deviceName}, transport_{serviceUuid.c_str(), rxUuid.c_str(), txUuid.c_str()},
          scheduler_{transport_} {}

    void addService(int32_t type, BleService* service) {
        services_.emplace(type, service);
    }

    void start() {
        transport_.onError([](const char* msg) {
            Serial.print("BTP Error: ");
            Serial.println(msg);
        });

        scheduler_.onDataReceived([this](uint8_t* data, size_t size) {
            if (size < 3) {
                Serial.println("BTP: Received data too short.");
                return;
            }

            const auto type = data[0];

            if (const auto iter = services_.find(type); iter != services_.end() && iter->second) {
                iter->second->run(type, std::span{data + 1, size - 1}, transport_);
            }
        });

        scheduler_.start(deviceName_.c_str());
    }

    void stop() {}

private:
    String deviceName_;
    Btp::BtpTransport transport_;
    Btp::BtpTransportScheduler scheduler_;
    std::unordered_map<uint8_t, BleService*> services_;
};

BleServer::BleServer(const String& deviceName, const String& serviceUuid, const String& rxUuid, const String& txUuid)
    : impl_{std::make_unique<impl>(deviceName, serviceUuid, rxUuid, txUuid)} {}

BleServer::BleServer(BleServer&&) noexcept = default;

BleServer::~BleServer() = default;

BleServer& BleServer::operator=(BleServer&&) noexcept = default;

void BleServer::addService(int32_t type, BleService* service) const {
    impl_->addService(type, service);
}

void BleServer::start() const {
    impl_->start();
}

void BleServer::stop() const {
    impl_->stop();
}
