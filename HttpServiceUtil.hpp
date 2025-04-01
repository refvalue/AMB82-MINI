#pragma once

#include "HttpMessage.hpp"

#include <stdint.h>

struct cJSON;

namespace HttpServiceUtil {
    void sendResponseBody(
        HttpMessage& response, bool success, int32_t code, const char* message, cJSON* data = nullptr);
};
