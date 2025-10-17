#pragma once

#include "DateTime.hpp"

#include <cstdint>

#include <WString.h>

namespace TimeUtil {
    int64_t toUnixTimestamp(const DateTime& dateTime) noexcept;
    int64_t toUnixTimestamp(
        int32_t year, int32_t month, int32_t day, int32_t hour, int32_t minute, int32_t second) noexcept;

    int32_t toTimestampSince2020(int64_t timestamp) noexcept;
    int32_t toTimestampSince2020(const DateTime& dateTime) noexcept;
    int64_t toUnixTimestampFromSince2020(int32_t timestampSince2020) noexcept;

    DateTime toDateTime(int64_t timestamp) noexcept;
    DateTime toDateTimeFromSince2020(int32_t timestampSince2020) noexcept;
    
    String toIso8601(const DateTime& dateTime);
} // namespace TimeUtil
