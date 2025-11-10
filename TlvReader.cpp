#include "TlvReader.hpp"

#include "BinaryUtil.hpp"
#include "TlvConstants.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

namespace {
    template <typename... Ts>
    struct overloaded : Ts... {
        using Ts::operator()...;
    };

    template <class... Ts>
    overloaded(Ts...) -> overloaded<Ts...>;

    constexpr uint8_t readU8(std::span<const uint8_t> buffer) noexcept {
        return buffer[0];
    }

    TlvReader::ReadHandler makeReadHandler(std::span<const uint8_t> buffer) {
        size_t offset{};

        return [buffer, offset](uint8_t* data, size_t size) mutable -> size_t {
            const auto bytesToRead = std::min(size, buffer.size() - offset);

            if (bytesToRead == 0) {
                return {};
            }

            std::ranges::copy(buffer.subspan(offset, bytesToRead), data);

            offset += bytesToRead;

            return bytesToRead;
        };
    }

    bool checkMagic(const TlvReader::ReadHandler& readHandler) {
        std::array<uint8_t, TlvConstants::magic.size()> buffer{};
        const auto bytesRead = readHandler(buffer.data(), buffer.size());

        if (bytesRead != buffer.size()) {
            return false;
        }

        return std::ranges::equal(buffer, TlvConstants::magic);
    }

    void dispatchValue(uint8_t type, std::span<const uint8_t> rawValue, const TlvReader::DataHandlerVariant& variant) {
        if (rawValue.empty()) {
            return;
        }

        auto createHandler = [&]<typename T, typename ReadBigHandler>(
                                 std::type_identity<T>, ReadBigHandler&& readHandler) {
            return [&](const TlvReader::DataHandler<T>& handler) {
                if (rawValue.size() == sizeof(T)) {
                    auto value = std::forward<ReadBigHandler>(readHandler)(rawValue);

                    handler(type, std::move(value));
                }
            };
        };

        std::visit(
            overloaded{
                createHandler(std::type_identity<uint8_t>{}, &readU8),
                createHandler(std::type_identity<uint16_t>{}, &BinaryUtil::readU16Be),
                createHandler(std::type_identity<uint32_t>{}, &BinaryUtil::readU32Be),
                createHandler(std::type_identity<uint64_t>{}, &BinaryUtil::readU64Be),
                [&](const TlvReader::DataHandler<String>& handler) {
                    String value{reinterpret_cast<const char*>(rawValue.data())};

                    handler(type, value);
                },
            },
            variant);
    }
} // namespace

TlvReader::TlvReader(std::span<const uint8_t> buffer) : TlvReader{makeReadHandler(buffer)} {}

TlvReader::TlvReader(ReadHandler readHandler) : readHandler_{std::move(readHandler)} {}

TlvReader::TlvReader(TlvReader&& other) noexcept
    : readHandler_{std::move(other.readHandler_)}, typeMapping_{std::move(other.typeMapping_)} {}

TlvReader::~TlvReader() = default;

TlvReader& TlvReader::operator=(TlvReader&& other) noexcept {
    if (this != &other) {
        readHandler_ = std::move(other.readHandler_);
        typeMapping_ = std::move(other.typeMapping_);
    }

    return *this;
}

void TlvReader::registerHandler(uint8_t type, DataHandlerVariant handler) {
    typeMapping_.insert_or_assign(type, std::move(handler));
}

bool TlvReader::readAll() const {
    static constexpr size_t bufferSize = 1024;

    std::array<uint8_t, bufferSize> buffer{};
    size_t bytesRead{};
    size_t offset{};

    if (!checkMagic(readHandler_)) {
        return false;
    }

    while ((bytesRead = readHandler_(buffer.data() + offset, bufferSize - offset)) != 0) {
        offset += bytesRead;

        size_t parseOffset{};

        while (parseOffset + TlvConstants::typeLengthSize <= offset) {
            const auto type   = buffer[parseOffset];
            const auto length = buffer[parseOffset + 1];

            if (parseOffset + TlvConstants::typeLengthSize + length > offset) {
                break;
            }

            if (const auto iter = typeMapping_.find(type); iter != typeMapping_.end()) {
                auto&& variant   = iter->second;
                const auto value = buffer.data() + parseOffset + TlvConstants::typeLengthSize;

                dispatchValue(type, std::span<const uint8_t>{value, length}, variant);
            }

            parseOffset += TlvConstants::typeLengthSize + length;
        }

        if (parseOffset < offset) {
            std::ranges::copy(buffer.data() + parseOffset, buffer.data() + offset, buffer.data());
            offset -= parseOffset;
        } else {
            offset = 0;
        }
    }

    return true;
}
