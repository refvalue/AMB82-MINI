#include "HttpServiceUtil.hpp"
#include "cJSON.hpp"

namespace HttpServiceUtil {
    void sendResponseBody(HttpMessage& response, bool success, int32_t code, const char* message, cJSON* data) {
        const cJSONPtr body{cJSON_CreateObject()};

        cJSON_AddBoolToObject(body.get(), "success", success);
        cJSON_AddNumberToObject(body.get(), "code", code);
        cJSON_AddStringToObject(body.get(), "message", message);

        if (data) {
            cJSON_AddItemReferenceToObject(body.get(), "data", data);
        } else {
            cJSON_AddNullToObject(body.get(), "data");
        }

        response.setHeader("Connection", "close");
        response.writeJson(body.get());
    }
} // namespace HttpServiceUtil
