#pragma once

#include <functional>
#include <string_view>

#include <WString.h>

namespace HashUtil {
    struct StringHash {
        std::size_t operator()(const String& str) const noexcept {
            return std::hash<std::string_view>{}(std::string_view{str.c_str(), str.length()});
        }
    };
} // namespace HashUtil
