#pragma once

#include "BinaryUtil.hpp"

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

#include <WString.h>

class TlvWriter {
public:
    TlvWriter();
    TlvWriter(const TlvWriter&);
    TlvWriter(TlvWriter&&) noexcept;
    ~TlvWriter();
    TlvWriter& operator=(const TlvWriter&);
    TlvWriter& operator=(TlvWriter&&) noexcept;
    std::span<const uint8_t> data() const noexcept;
    void clear();
    void write(uint8_t type, std::span<const uint8_t> value);
    void write(uint8_t type, uint8_t value);
    void write(uint8_t type, uint16_t value);
    void write(uint8_t type, uint32_t value);
    void write(uint8_t type, uint64_t value);
    void write(uint8_t type, const String& value, size_t maxSize);

private:
    std::vector<uint8_t> buffer_;
};
