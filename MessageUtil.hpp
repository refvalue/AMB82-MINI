#pragma once

#include <cstdint>

struct cJSON;
struct HttpMessage;

namespace Btp {
    class BtpTransport;
}

namespace MessageUtil {
    void sendResponseBody(
        HttpMessage& response, bool success, int32_t code, const char* message, cJSON* data = nullptr);

    void sendResponseBody(
        Btp::BtpTransport& transport, bool success, int32_t code, const char* message, cJSON* data = nullptr);
}; // namespace MessageUtil
