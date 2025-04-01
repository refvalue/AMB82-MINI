#pragma once

#include <WString.h>
#include <stddef.h>
#include <stdint.h>

namespace CryptoUtil {
    String sha1(const uint8_t* buffer, size_t size, bool base64 = false);
    String sha1(const String& str, bool base64 = false);

    String sha256(const uint8_t* buffer, size_t size, bool base64 = false);
    String sha256(const String& str, bool base64 = false);
}; // namespace CryptoUtil
