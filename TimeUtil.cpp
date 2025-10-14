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
} // namespace TimeUtil
