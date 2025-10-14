#pragma once

#include "DateTime.hpp"

#include <cstdint>

namespace TimeUtil {
    int64_t toUnixTimestamp(const DateTime& dateTime) noexcept;
    int64_t toUnixTimestamp(
        int32_t year, int32_t month, int32_t day, int32_t hour, int32_t minute, int32_t second) noexcept;
} // namespace TimeUtil
