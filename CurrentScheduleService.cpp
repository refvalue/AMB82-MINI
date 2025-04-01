#include "AppConfig.hpp"
#include "HttpMessageServer.hpp"
#include "HttpService.hpp"
#include "HttpServiceUtil.hpp"
#include "Resources.hpp"
#include "cJSON.hpp"

#include <Client.h>
#include <WString.h>
#include <stddef.h>

namespace {
    struct CurrentScheduleService : HttpService {
        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            HttpMessageServer response{client};

            cJSONPtr data{cJSON_CreateObject()};

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            const auto schedule = globalAppConfig.value.getScheduleJson();
            xSemaphoreGive(globalAppMutex);

            cJSON_AddItemReferenceToObject(data.get(), "schedule", schedule.get());
            HttpServiceUtil::sendResponseBody(response, true, 0, "Success", data.get());
        }
    };
} // namespace

auto&& currentScheduleService = []() -> HttpService& {
    static CurrentScheduleService service;

    return service;
}();
