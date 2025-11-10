#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <unordered_map>
#include <variant>

#include <WString.h>

class TlvReader {
public:
    using ReadHandler = std::function<size_t(uint8_t* data, size_t size)>;

    template <typename T>
    using DataHandler = std::function<void(uint8_t type, T value)>;

    using DataHandlerVariant = std::variant<DataHandler<uint8_t>, DataHandler<uint16_t>, DataHandler<uint32_t>,
        DataHandler<uint64_t>, DataHandler<String>>;

    explicit TlvReader(std::span<const uint8_t> buffer);
    explicit TlvReader(ReadHandler readHandler);
    TlvReader(const TlvReader&) = delete;
    TlvReader(TlvReader&&) noexcept;
    ~TlvReader();
    TlvReader& operator=(const TlvReader&) = delete;
    TlvReader& operator=(TlvReader&&) noexcept;

    void registerHandler(uint8_t type, DataHandlerVariant handler);
    bool readAll() const;

private:
    ReadHandler readHandler_;
    std::unordered_map<uint8_t, DataHandlerVariant> typeMapping_;
};
