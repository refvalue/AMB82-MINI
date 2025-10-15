#include "TimeUtil.hpp"

namespace TimeUtil {
    int64_t toUnixTimestamp(const DateTime& dateTime) noexcept {
        return toUnixTimestamp(
            dateTime.year, dateTime.month, dateTime.day, dateTime.hour, dateTime.minute, dateTime.second);
    }

    int64_t toUnixTimestamp(int32_t year, int32_t month, int32_t day, int32_t hour, int32_t min, int32_t sec) noexcept {
        if (month <= 2) {
            year -= 1;
            month += 12;
        }

        const auto era  = year / 400LL;
        const auto yoe  = year - era * 400LL; // [0, 399]
        const auto doy  = (153LL * (month - 3) + 2) / 5 + day - 1; // [0, 365]
        const auto doe  = yoe * 365LL + yoe / 4 - yoe / 100 + doy; // [0, 146096]
        const auto days = era * 146097LL + doe - 719468LL; // days since 1970-01-01

        return days * 86400LL + hour * 3600LL + min * 60LL + sec;
    }

    DateTime fromUnixTimestamp(int64_t timestamp) noexcept {
        DateTime result;

        auto days = timestamp / 86400LL + 719468LL;
        auto secs = timestamp % 86400LL;

        if (secs < 0) {
            secs += 86400LL;
            days -= 1;
        }

        result.hour   = static_cast<uint8_t>(secs / 3600LL);
        secs          = secs % 3600LL;
        result.minute = static_cast<uint8_t>(secs / 60LL);
        result.second = static_cast<uint8_t>(secs % 60LL);

        const auto era = (days >= 0 ? days : days - 146096LL) / 146097LL;
        const auto doe = days - era * 146097LL; // [0, 146096]
        const auto yoe = (doe - doe / 1460LL + doe / 36524LL - doe / 146096LL) / 365LL; // [0, 399]
        const auto y   = yoe + era * 400LL;
        const auto doy = doe - (365LL * yoe + yoe / 4 - yoe / 100); // [0, 365]
        const auto mp  = (5LL * doy + 2) / 153LL; // [0, 11]

        result.day   = static_cast<uint8_t>(doy - (153LL * mp + 2) / 5 + 1); // [1, 31]
        result.month = static_cast<uint8_t>(mp < 10 ? mp + 3 : mp - 9); // [1, 12]
        result.year  = static_cast<uint16_t>(y + (result.month <= 2 ? 1 : 0));

        return result;
    }
} // namespace TimeUtil
