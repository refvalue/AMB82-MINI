#include "HttpMessageServer.hpp"
#include "HttpService.hpp"
#include "HttpServiceUtil.hpp"
#include "Resources.hpp"
#include "cJSON.hpp"

#include <Client.h>
#include <WString.h>
#include <rtc.h>
#include <task.h>

namespace {
    struct UpdateTimeService : HttpService {
        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            const auto params = message.getBodyAsJson();
            HttpMessageServer response{client};

            if (cJSON_IsNull(params.get()) || cJSON_IsInvalid(params.get())) {
                return HttpServiceUtil::sendResponseBody(response, "failure", -1, "Invalid request body.");
            }

            if (const auto item = cJSON_GetObjectItem(params.get(), "timestamp"); item && cJSON_IsNumber(item)) {
                const auto timestamp = cJSON_GetNumberValue(item);

                xSemaphoreTake(globalAppMutex, portMAX_DELAY);
                rtc.Write(timestamp);
                xSemaphoreGive(globalAppMutex);

                return HttpServiceUtil::sendResponseBody(response, true, 0, "RTC time successfully synchornized.");
            }

            HttpServiceUtil::sendResponseBody(response, false, -2, "Missing numeric parameter: `timestamp`.");
        }
    };
} // namespace

auto&& updateTimeService = []() -> HttpService& {
    static UpdateTimeService service;

    return service;
}();
