#include "BleServer.hpp"

#include "BleService.hpp"
#include "BtpTransport.hpp"
#include "BtpTransportScheduler.hpp"
#include "cJSON.hpp"

#include <string>
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
        transport_.onDataReceived([this](uint8_t* data, size_t size) {
            const cJSONPtr root{cJSON_Parse(std::string{reinterpret_cast<char*>(data), size}.c_str())};

            if (!root) {
                return;
            }

            const auto typeItem = cJSON_GetObjectItem(root.get(), "type");

            if (typeItem == nullptr || !cJSON_IsNumber(typeItem)) {
                return;
            }

            const auto type     = typeItem->valueint;
            const auto dataItem = cJSON_GetObjectItem(root.get(), "data");
            const auto iter     = services_.find(type);

            if (iter != services_.end() && iter->second) {
                iter->second->run(type, dataItem, transport_);
            }
        });

        transport_.onError([](const char* msg) {
            Serial.print("BTP Error: ");
            Serial.println(msg);
        });

        scheduler_.start(deviceName_.c_str());
    }

    void stop() {
        scheduler_.stop();
    }

private:
    String deviceName_;
    Btp::BtpTransport transport_;
    Btp::BtpTransportScheduler scheduler_;
    std::unordered_map<int32_t, BleService*> services_;
};

BleServer::BleServer(const String& deviceName, const String& serviceUuid, const String& rxUuid, const String& txUuid)
    : impl_{std::make_unique<impl>(deviceName, serviceUuid, rxUuid, txUuid)} {}

BleServer::BleServer(BleServer&&) noexcept = default;

BleServer::~BleServer() = default;

BleServer& BleServer::operator=(BleServer&&) noexcept = default;

void BleServer::addService(int32_t type, BleService* service) {
    impl_->addService(type, service);
}

void BleServer::start() {
    impl_->start();
}

void BleServer::stop() {
    impl_->stop();
}
