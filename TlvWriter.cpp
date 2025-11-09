#include "TlvWriter.hpp"

#include "TlvConstants.hpp"

#include <algorithm>
#include <array>
#include <type_traits>
#include <utility>

namespace {
    void writeMagicIfNeeded(std::vector<uint8_t>& buffer) {
        if (buffer.empty()) {
            buffer.insert(buffer.end(), TlvConstants::magic.begin(), TlvConstants::magic.end());
        }
    }

    void writeRaw(std::vector<uint8_t>& buffer, uint8_t type, std::span<const uint8_t> value) {
        writeMagicIfNeeded(buffer);
        buffer.emplace_back(type);
        buffer.emplace_back(static_cast<uint8_t>(value.size()));
        buffer.insert(buffer.end(), value.begin(), value.end());
    }

    template <typename T>
        requires std::is_arithmetic_v<T>
    void writePritimive(
        std::vector<uint8_t>& buffer, uint8_t type, T value, void (*writeHandler)(std::span<uint8_t, sizeof(T)>, T)) {
        std::array<uint8_t, sizeof(T)> intermediate{};

        writeHandler(intermediate, value);
        writeRaw(buffer, type, intermediate);
    }
} // namespace

TlvWriter::TlvWriter() = default;

TlvWriter::TlvWriter(const TlvWriter& other) : buffer_{other.buffer_} {}

TlvWriter::TlvWriter(TlvWriter&& other) noexcept : buffer_{std::move(other.buffer_)} {}

TlvWriter::~TlvWriter() = default;

TlvWriter& TlvWriter::operator=(const TlvWriter& other) {
    if (this != &other) {
        buffer_ = other.buffer_;
    }

    return *this;
}

TlvWriter& TlvWriter::operator=(TlvWriter&& other) noexcept {
    if (this != &other) {
        buffer_ = std::move(other.buffer_);
    }

    return *this;
}

std::span<const uint8_t> TlvWriter::data() const noexcept {
    return buffer_;
}

void TlvWriter::clear() {
    buffer_.clear();
}

void TlvWriter::write(uint8_t type, std::span<const uint8_t> value) {
    writeRaw(buffer_, type, value);
}

void TlvWriter::write(uint8_t type, uint8_t value) {
    write(type, std::span<const uint8_t>{&value, 1});
}

void TlvWriter::write(uint8_t type, uint16_t value) {
    writePritimive(buffer_, type, value, &BinaryUtil::writeU16Be);
}

void TlvWriter::write(uint8_t type, uint32_t value) {
    writePritimive(buffer_, type, value, &BinaryUtil::writeU32Be);
}

void TlvWriter::write(uint8_t type, uint64_t value) {
    writePritimive(buffer_, type, value, &BinaryUtil::writeU64Be);
}

void TlvWriter::write(uint8_t type, const String& value, size_t maxSize) {
    const auto size = std::min(value.length(), maxSize) + 1;

    write(type, std::span<const uint8_t>{reinterpret_cast<const uint8_t*>(value.c_str()), size});
}
