#include "BtpTransport.hpp"

#include "BtpConstants.hpp"
#include "ManagedTask.hpp"
#include "MessageQueue.hpp"

#include <algorithm>
#include <ranges>
#include <utility>

#include <BLEDevice.h>

namespace Btp {
    namespace {
        const BtpTransport* btpInstance{};
    }

    class BtpTransport::impl {
    public:
        impl(const char* serviceUuid, const char* rxUuid, const char* txUuid)
            : service_{serviceUuid}, rxChar_{rxUuid}, txChar_{txUuid}, notifyEnabled_{}, connId_{-1},
              rxBuffer_{std::make_unique<uint8_t[]>(Constants::mtu)} {}

        bool begin(const char* deviceName) {
            adv_.addFlags();
            adv_.addCompleteName(deviceName);
            scan_.addCompleteServices(service_.getUUID());

            rxChar_.setWriteProperty(true);
            rxChar_.setWritePermissions(GATT_PERM_WRITE);
            rxChar_.setWriteCallback(&impl::staticWriteCb);
            rxChar_.setBufferLen(static_cast<uint16_t>(Constants::mtu));

            txChar_.setReadProperty(true);
            txChar_.setReadPermissions(GATT_PERM_READ);
            txChar_.setNotifyProperty(true);
            txChar_.setCCCDCallback(&impl::staticNotifyCb);
            txChar_.setBufferLen(static_cast<uint16_t>(Constants::mtu));

            service_.addCharacteristic(rxChar_);
            service_.addCharacteristic(txChar_);

            BLE.init();
            BLE.configAdvert()->setAdvData(adv_);
            BLE.configAdvert()->setScanRspData(scan_);
            BLE.configServer(1);
            BLE.addService(service_);
            BLE.beginPeripheral();

            return true;
        }

        bool send(const uint8_t* data, size_t size) {
            if (size > Constants::mtu) {
                if (onError_) {
                    onError_("Data size exceeds MTU.");
                }

                return false;
            }

            txNotifyRaw(data, size, connId_ >= 0 ? connId_ : 0);

            return true;
        }

        bool send(std::span<const uint8_t> data) {
            return send(data.data(), data.size());
        }

        void onDataReceived(DataHandler handler) {
            onDataReceived_ = std::move(handler);
        }

        void onError(ErrorHandler handler) {
            onError_ = std::move(handler);
        }

        int32_t getConnID() const {
            return connId_;
        }

    private:
        void handleRxWrite(BLECharacteristic* chr, uint8_t connId) {
            connId_         = connId;
            const auto size = chr->getDataLen();

            if (size != 0) {
                chr->getData(rxBuffer_.get(), std::min<size_t>(size, Constants::mtu));

                if (onDataReceived_) {
                    onDataReceived_(rxBuffer_.get(), size);
                }
            }
        }

        void handleTxCccd(BLECharacteristic* chr, uint8_t connId, uint16_t cccd) {
            connId_        = connId;
            notifyEnabled_ = (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY);
        }

        void txNotifyRaw(const uint8_t* data, size_t size, int32_t connId) {
            if (data && size != 0) {
                txChar_.setData(const_cast<uint8_t*>(data), static_cast<uint16_t>(size));
                txChar_.notify(connId >= 0 ? connId : 0);
            }
        }

        static void staticWriteCb(BLECharacteristic* chr, uint8_t connId) {
            if (btpInstance) {
                btpInstance->impl_->handleRxWrite(chr, connId);
            }
        }

        static void staticNotifyCb(BLECharacteristic* chr, uint8_t connId, uint16_t cccd) {
            if (btpInstance) {
                btpInstance->impl_->handleTxCccd(chr, connId, cccd);
            }
        }

        BLEService service_;
        BLECharacteristic rxChar_;
        BLECharacteristic txChar_;
        BLEAdvertData adv_;
        BLEAdvertData scan_;

        bool notifyEnabled_;
        int32_t connId_;

        std::unique_ptr<uint8_t[]> rxBuffer_;

        DataHandler onDataReceived_;
        ErrorHandler onError_;
    };

    BtpTransport::BtpTransport(const char* serviceUuid, const char* rxUuid, const char* txUuid)
        : impl_{std::make_unique<impl>(serviceUuid, rxUuid, txUuid)} {}

    BtpTransport::BtpTransport(BtpTransport&&) noexcept = default;

    BtpTransport::~BtpTransport() = default;

    BtpTransport& BtpTransport::operator=(BtpTransport&&) noexcept = default;

    bool BtpTransport::begin(const char* deviceName) const {
        btpInstance = this;

        return impl_->begin(deviceName);
    }

    bool BtpTransport::send(const uint8_t* data, size_t size) const {
        return impl_->send(data, size);
    }

    bool BtpTransport::send(std::span<const uint8_t> data) const {
        return impl_->send(data);
    }

    void BtpTransport::onDataReceived(DataHandler handler) const {
        impl_->onDataReceived(std::move(handler));
    }

    void BtpTransport::onError(ErrorHandler handler) const {
        impl_->onError(std::move(handler));
    }

    int32_t BtpTransport::getConnID() const noexcept {
        return impl_->getConnID();
    }
} // namespace Btp
