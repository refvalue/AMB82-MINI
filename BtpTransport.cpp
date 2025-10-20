#include "BtpTransport.hpp"

#include "BtpConstants.hpp"
#include "BtpUtils.hpp"

#include <algorithm>
#include <array>
#include <cstring>
#include <ranges>
#include <span>
#include <utility>

#include <BLEDevice.h>

#if 1
#include <FreeRTOS.h>
#endif

#include <portmacro.h>
#include <semphr.h>

namespace Btp {
    namespace {
        const BtpTransport* btpInstance{};
    }

    class BtpTransport::impl {
    public:
        impl(const char* serviceUuid, const char* rxUuid, const char* txUuid)
            : mutex_{xSemaphoreCreateMutex()}, service_{serviceUuid}, rxChar_{rxUuid}, txChar_{txUuid},
              notifyEnabled_{}, connId_{-1}, rxExpectedLen_{}, rxMaxChunks_{}, lastReceiveMillis_{}, awaitingAck_{},
              awaitingAckSeq_{}, awaitingSince_{}, txSeq_{} {}

        ~impl() {
            freeReceiveState();
        }

        bool begin(const char* deviceName) {
            adv_.addFlags(GAP_ADTYPE_FLAGS_LIMITED | GAP_ADTYPE_FLAGS_BREDR_NOT_SUPPORTED);
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

        void poll() {
            xSemaphoreTake(mutex_, portMAX_DELAY);

            if (rxBuffer_) {
                if (millis() - lastReceiveMillis_ > Constants::receiveTimeoutMs) {
                    freeReceiveState();

                    if (onError_)
                        onError_("Receive timeout.");
                }
            }

            if (awaitingAck_) {
                if (millis() - awaitingSince_ > Constants::ackTimeoutMs) {
                }
            }

            xSemaphoreGive(mutex_);
        }

        bool send(const uint8_t* data, size_t size) {
            const auto start = millis();
            size_t offset{};
            auto first = true;

            while (offset < size) {
                const auto headerSize = Constants::headerSize + (first ? Constants::extraHeaderSize : 0);
                const auto chunkSize  = std::min(Constants::mtu - headerSize, size - offset);
                const auto frameSize  = headerSize + chunkSize;
                {
                    const auto frame = std::make_unique<uint8_t[]>(frameSize);

                    writeU16Be(frame.get(), txSeq_);
                    frame[2] = static_cast<uint8_t>(chunkSize);
                    frame[3] = static_cast<uint8_t>(PacketType::Data);

                    if (first) {
                        writeU32Be(frame.get() + Constants::headerSize, size);
                        first = false;
                    }

                    std::memcpy(frame.get() + headerSize, data + offset, chunkSize);
                    txNotifyRaw(frame.get(), frameSize, connId_ >= 0 ? connId_ : 0);
                }

                awaitingAck_    = true;
                awaitingAckSeq_ = txSeq_;
                awaitingSince_  = millis();

                if (!waitForAck(data, offset, size, start)) {
                    return false;
                }

                ++txSeq_;
                offset += chunkSize;
            }

            return true;
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
        bool waitForAck(const uint8_t* data, size_t offset, size_t size, uint32_t startTimestamp) {
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
                    const auto chunkSize  = std::min((size_t) (Constants::mtu - headerSize), size - offset);
                    const auto frameSize  = headerSize + chunkSize;
                    const auto frame      = std::make_unique<uint8_t[]>(frameSize);

                    writeU16Be(frame.get(), txSeq_);
                    frame[2] = static_cast<uint8_t>(chunkSize);
                    frame[3] = static_cast<uint8_t>(PacketType::Data);

                    if (txSeq_ == 0) {
                        writeU32Be(frame.get() + Constants::headerSize, size);
                    }

                    std::memcpy(frame.get() + headerSize, data + offset, chunkSize);
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

        void processAck(uint16_t seq) {
            xSemaphoreTake(mutex_, portMAX_DELAY);

            if (awaitingAck_ && seq == awaitingAckSeq_) {
                awaitingAck_ = false;
            }

            xSemaphoreGive(mutex_);
        }

        void processIncomingData(uint16_t seq, uint8_t payloadSize, std::span<const uint8_t> data) {
            auto payloadOffset = Constants::headerSize;

            if (seq == 0) {
                if (data.size() < Constants::headerSize + Constants::extraHeaderSize) {
                    return;
                }

                const auto expected = readU32Be(data.data() + Constants::headerSize);

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
            auto copySize     = payloadSize;

            if (offset + copySize > rxExpectedLen_) {
                copySize = offset >= rxExpectedLen_ ? 0 : rxExpectedLen_ - offset;
            }

            if (copySize > 0) {
                std::memcpy(rxBuffer_.get() + offset, data.data() + payloadOffset, copySize);
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

            if (const auto all = std::ranges::all_of(
                    rxFlags_.get(), rxFlags_.get() + rxMaxChunks_, [](auto inner) { return inner; })) {
                if (onDataReceived_) {
                    onDataReceived_(rxBuffer_.get(), rxExpectedLen_);
                }

                freeReceiveState();
            }
        }

        void handleRxWrite(BLECharacteristic* chr, uint8_t connId) {
            connId_   = connId;
            auto size = chr->getDataLen();

            if (size <= 0 || size < Constants::headerSize) {
                return;
            }
            const auto buffer      = chr->getDataBuff();
            const auto seq         = readU16Be(buffer);
            const auto payloadSize = buffer[2];
            const auto type        = static_cast<PacketType>(buffer[3]);

            switch ((type)) {
            case PacketType::Ack:
                xSemaphoreTake(mutex_, portMAX_DELAY);
                processAck(seq);
                xSemaphoreGive(mutex_);
                break;
            case PacketType::Data:
                xSemaphoreTake(mutex_, portMAX_DELAY);
                processIncomingData(seq, payloadSize, std::span{buffer, size});
                xSemaphoreGive(mutex_);
                break;
            default:
                break;
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

        SemaphoreHandle_t mutex_;
        BLEService service_;
        BLECharacteristic rxChar_;
        BLECharacteristic txChar_;
        BLEAdvertData adv_;
        BLEAdvertData scan_;

        bool notifyEnabled_;
        int32_t connId_;

        size_t rxExpectedLen_;
        size_t rxMaxChunks_;
        uint32_t lastReceiveMillis_;
        std::unique_ptr<bool[]> rxFlags_;
        std::unique_ptr<uint8_t[]> rxBuffer_;

        bool awaitingAck_;
        uint16_t awaitingAckSeq_;
        uint32_t awaitingSince_;

        uint16_t txSeq_;

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

    void BtpTransport::poll() const {
        impl_->poll();
    }

    bool BtpTransport::send(const uint8_t* data, size_t size) const {
        return impl_->send(data, size);
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
