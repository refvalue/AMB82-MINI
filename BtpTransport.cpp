#include "BtpTransport.hpp"

#include "BtpConstants.hpp"
#include "BtpUtils.hpp"

#include <cstring>
#include <utility>

#include <BLEDevice.h>

#if 1
#include <FreeRTOS.h>
#endif

#include <portmacro.h>

namespace Btp {
    namespace {
        const BtpTransport* btpInstance{};
    }

    class BtpTransport::impl {
    public:
        impl(const char* serviceUuid, const char* rxUuid, const char* txUuid)
            : service_{serviceUuid}, rxChar_{rxUuid}, txChar_{txUuid}, notifyEnabled_{}, connId_{-1}, rxExpectedLen_{},
              rxMaxChunks_{}, lastReceiveMillis_{}, awaitingAck_{}, awaitingAckSeq_{}, awaitingSince_{}, txSeq_{} {}

        ~impl() {
            freeReceiveState();
        }

        bool begin(const char* deviceName) {
            BLE.init();

            adv_.addFlags(GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED);
            adv_.addCompleteName(deviceName);
            scan_.addCompleteServices(BLEUUID(service_.getUUID().str()));

            rxChar_.setWriteProperty(true);
            rxChar_.setWritePermissions(GATT_PERM_WRITE);
            rxChar_.setWriteCallback(&impl::staticWriteCb);
            rxChar_.setBufferLen(Constants::mtu);

            txChar_.setReadProperty(true);
            txChar_.setReadPermissions(GATT_PERM_READ);
            txChar_.setNotifyProperty(true);
            txChar_.setCCCDCallback(&impl::staticNotifCb);
            txChar_.setBufferLen(Constants::mtu);

            service_.addCharacteristic(rxChar_);
            service_.addCharacteristic(txChar_);

            BLE.configAdvert()->setAdvData(adv_);
            BLE.configAdvert()->setScanRspData(scan_);
            BLE.configServer(1);
            BLE.addService(service_);

            BLE.beginPeripheral();

            return true;
        }

        void poll() {
            if (rxBuffer_) {
                if (millis() - lastReceiveMillis_ > Constants::receiveTimeoutMs) {
                    freeReceiveState();

                    if (onError_)
                        onError_("Receive timeout");
                }
            }

            if (awaitingAck_) {
                if (millis() - awaitingSince_ > Constants::ackTimeoutMs) {
                }
            }
        }

        bool send(const uint8_t* data, size_t length) {
            const auto start = millis();
            size_t offset{};
            auto first = true;

            while (offset < length) {
                const auto headerSize = Constants::headerSize + (first ? Constants::extraHeaderSize : 0);
                const auto chunkLen   = min(Constants::mtu - headerSize, length - offset);
                const auto frameSize  = headerSize + chunkLen;
                {
                    const auto frame = std::make_unique<uint8_t[]>(frameSize);

                    writeU16Be(frame.get(), txSeq_);
                    frame[2] = static_cast<uint8_t>(chunkLen);
                    frame[3] = static_cast<uint8_t>(PacketType::Data);

                    if (first) {
                        writeU32Be(frame.get() + Constants::headerSize, length);
                        first = false;
                    }

                    memcpy(frame.get() + headerSize, data + offset, chunkLen);
                    txNotifyRaw(frame.get(), frameSize, connId_ >= 0 ? connId_ : 0);
                }

                awaitingAck_    = true;
                awaitingAckSeq_ = txSeq_;
                awaitingSince_  = millis();

                if (!waitForAck(data, offset, length, start)) {
                    return false;
                }

                ++txSeq_;
                offset += chunkLen;
            }

            return true;
        }

        void setOnDataReceived(DataCallback cb) {
            onDataReceived_ = std::move(cb);
        }

        void setOnError(ErrorCallback cb) {
            onError_ = std::move(cb);
        }

        int32_t getConnID() const {
            return connId_;
        }

    private:
        bool waitForAck(const uint8_t* data, size_t offset, size_t length, uint32_t startTimestamp) {
            for (;;) {
                if (!awaitingAck_) {
                    return true;
                }

                if (millis() - awaitingSince_ > Constants::ackTimeoutMs) {
                    if (millis() - startTimestamp > Constants::transferTimeoutMs) {
                        if (onError_) {
                            onError_("Transfer timeout.");
                        }

                        awaitingAck_ = false;
                        return false;
                    }

                    const auto headerSize = Constants::headerSize + (txSeq_ == 0 ? Constants::extraHeaderSize : 0);

                    const auto chunkLen  = min((size_t) (Constants::mtu - headerSize), length - offset);
                    const auto frameSize = headerSize + chunkLen;
                    const auto frame     = std::make_unique<uint8_t[]>(frameSize);

                    writeU16Be(frame.get(), txSeq_);
                    frame[2] = static_cast<uint8_t>(chunkLen);
                    frame[3] = static_cast<uint8_t>(PacketType::Data);

                    if (txSeq_ == 0) {
                        writeU32Be(frame.get() + Constants::headerSize, length);
                    }

                    memcpy(frame.get() + headerSize, data + offset, chunkLen);
                    txNotifyRaw(frame.get(), frameSize, connId_ >= 0 ? connId_ : 0);

                    awaitingSince_ = millis();
                }

                vTaskDelay(pdMS_TO_TICKS(5));
            }
        }

        void freeReceiveState() {
            rxBuffer_.reset();
            rxFlags_.reset();

            rxExpectedLen_     = 0;
            rxMaxChunks_       = 0;
            lastReceiveMillis_ = 0;
        }

        void allocReceiveBuffers(size_t expected) {
            rxExpectedLen_ = expected;
            rxMaxChunks_   = (rxExpectedLen_ + Constants::maxPayload - 1) / Constants::maxPayload;
            rxBuffer_      = std::make_unique<uint8_t[]>(rxExpectedLen_);
            rxFlags_       = std::make_unique<bool[]>(rxMaxChunks_);
        }

        void handleRxWrite(BLECharacteristic* chr, uint8_t connId) {
            connId_  = connId;
            auto len = chr->getDataLen();

            if (len <= 0) {
                return;
            }

            uint8_t tmp[512];

            if (len > sizeof(tmp)) {
                len = sizeof(tmp);
            }

            chr->getData(tmp, len);

            if (len < Constants::headerSize) {
                return;
            }

            const auto seq        = readU16Be(tmp);
            const auto payloadLen = tmp[2];
            const auto type       = static_cast<PacketType>(tmp[3]);

            if (type == PacketType::Ack) {
                if (awaitingAck_ && seq == awaitingAckSeq_) {
                    awaitingAck_ = false;
                }
                return;
            }

            if (type == PacketType::Data) {
                auto payloadOffset = Constants::headerSize;

                if (seq == 0) {
                    if (len < Constants::headerSize + Constants::extraHeaderSize) {
                        return;
                    }

                    const auto expected = readU32Be(tmp + Constants::headerSize);

                    if (expected == 0) {
                        return;
                    }

                    freeReceiveState();
                    allocReceiveBuffers(expected);
                    payloadOffset += Constants::extraHeaderSize;
                } else if (!rxBuffer_) {
                    return;
                }

                const auto offset = seq * Constants::maxPayload;
                auto copyLen      = payloadLen;

                if (offset + copyLen > rxExpectedLen_) {
                    copyLen = offset >= rxExpectedLen_ ? 0 : rxExpectedLen_ - offset;
                }

                if (copyLen > 0) {
                    memcpy(rxBuffer_.get() + offset, tmp + payloadOffset, copyLen);
                }

                if (seq < rxMaxChunks_) {
                    rxFlags_[seq] = true;
                }

                lastReceiveMillis_ = millis();

                uint8_t ack[Constants::headerSize];
                writeU16Be(ack, seq);

                ack[2] = 0;
                ack[3] = static_cast<uint8_t>(PacketType::Ack);
                txNotifyRaw(ack, Constants::headerSize, connId_);

                auto all = true;

                for (size_t i = 0; i < rxMaxChunks_; i++) {
                    if (!rxFlags_[i]) {
                        all = false;
                        break;
                    }
                }

                if (all) {
                    if (onDataReceived_) {
                        onDataReceived_(rxBuffer_.get(), rxExpectedLen_);
                    }

                    freeReceiveState();
                }
            }
        }

        void handleTxCccd(BLECharacteristic* chr, uint8_t connId, uint16_t cccd) {
            connId_        = connId;
            notifyEnabled_ = (cccd & GATT_CLIENT_CHAR_CONFIG_NOTIFY);
        }

        void txNotifyRaw(const uint8_t* buf, size_t len, int32_t connId) {
            if (len == 0) {
                return;
            }

            txChar_.setData(const_cast<uint8_t*>(buf), static_cast<uint16_t>(len));
            txChar_.notify(connId >= 0 ? connId : 0);
        }

        // static wrappers for BLE callbacks
        static void staticWriteCb(BLECharacteristic* chr, uint8_t connId) {
            if (btpInstance) {
                btpInstance->impl_->handleRxWrite(chr, connId);
            }
        }

        static void staticNotifCb(BLECharacteristic* chr, uint8_t connId, uint16_t cccd) {
            if (btpInstance) {
                btpInstance->impl_->handleTxCccd(chr, connId, cccd);
            }
        }

        BLEService service_;
        BLECharacteristic rxChar_;
        BLECharacteristic txChar_;
        BLEAdvertData adv_;
        BLEAdvertData scan_;

        volatile bool notifyEnabled_;
        volatile int connId_;

        size_t rxExpectedLen_;
        size_t rxMaxChunks_;
        uint32_t lastReceiveMillis_;
        std::unique_ptr<bool[]> rxFlags_;
        std::unique_ptr<uint8_t[]> rxBuffer_;

        bool awaitingAck_;
        uint16_t awaitingAckSeq_;
        uint32_t awaitingSince_;

        uint16_t txSeq_;

        DataCallback onDataReceived_;
        ErrorCallback onError_;
    };

    bool BtpTransport::begin(const char* deviceName) const {
        btpInstance = this;

        return impl_->begin(deviceName);
    }

    void BtpTransport::poll() const {
        impl_->poll();
    }

    bool BtpTransport::send(const uint8_t* data, size_t length) const {
        return impl_->send(data, length);
    }

    void BtpTransport::setOnDataReceived(DataCallback cb) const {
        impl_->setOnDataReceived(std::move(cb));
    }

    void BtpTransport::setOnError(ErrorCallback cb) const {
        impl_->setOnError(std::move(cb));
    }

    int32_t BtpTransport::getConnID() const noexcept {
        return impl_->getConnID();
    }

    BtpTransport::BtpTransport(const char* serviceUuid, const char* rxUuid, const char* txUuid)
        : impl_{std::make_unique<impl>(serviceUuid, rxUuid, txUuid)} {}

    BtpTransport::~BtpTransport() = default;
} // namespace Btp
