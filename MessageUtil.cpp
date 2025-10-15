#include "MessageUtil.hpp"

#include "BtpTransport.hpp"
#include "HttpMessage.hpp"
#include "cJSON.hpp"

#include <cstring>
#include <memory>

namespace MessageUtil {
    namespace {
        using JsonStringPtr = std::unique_ptr<char[], decltype(&cJSON_free)>;

        cJSONPtr createBody(bool success, int32_t code, const char* message, cJSON* data) {
            const cJSONPtr body{cJSON_CreateObject()};

            cJSON_AddBoolToObject(body.get(), "success", success);
            cJSON_AddNumberToObject(body.get(), "code", code);
            cJSON_AddStringToObject(body.get(), "message", message);

            if (data) {
                cJSON_AddItemReferenceToObject(body.get(), "data", data);
            } else {
                cJSON_AddNullToObject(body.get(), "data");
            }

            return body;
        }
    } // namespace

    void sendResponseBody(HttpMessage& response, bool success, int32_t code, const char* message, cJSON* data) {
        response.setHeader("Connection", "close");
        response.writeJson(createBody(success, code, message, data).get());
    }

    void sendResponseBody(Btp::BtpTransport& transport, bool success, int32_t code, const char* message, cJSON* data) {
        if (const JsonStringPtr jsonStr{
                cJSON_PrintUnformatted(createBody(success, code, message, data).get()), &cJSON_free}) {
            transport.send(reinterpret_cast<const uint8_t*>(jsonStr.get()), std::strlen(jsonStr.get()));
        }
    }
} // namespace MessageUtil
