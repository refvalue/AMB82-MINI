#include "AppConfig.hpp"
#include "HttpMessageServer.hpp"
#include "HttpService.hpp"
#include "HttpServiceUtil.hpp"
#include "Resources.hpp"
#include "SafeList.hpp"
#include "cJSON.hpp"

#include <utility>

#include <Client.h>
#include <WString.h>
#include <stdint.h>

namespace {
    struct UpdateScheduleService : HttpService {
        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            const auto params = message.getBodyAsJson();
            HttpMessageServer response{client};

            if (cJSON_IsNull(params.get()) || cJSON_IsInvalid(params.get())) {
                return HttpServiceUtil::sendResponseBody(response, "failure", -1, "Invalid request body.");
            }

            if (const auto schedule = cJSON_GetObjectItem(params.get(), "schedule");
                schedule && cJSON_IsArray(schedule)) {
                SafeList<AppConfig::RecordingPlan> scheduleList;
                cJSON* startTimestamp{};
                cJSON* duration{};

                for (auto item = schedule->child; item; item = item->next) {
                    if (!cJSON_IsObject(item)
                        || (startTimestamp = cJSON_GetObjectItem(item, "startTimestamp")) == nullptr
                        || (duration = cJSON_GetObjectItem(item, "duration")) == nullptr
                        || !cJSON_IsNumber(startTimestamp) || !cJSON_IsNumber(duration)) {
                        return HttpServiceUtil::sendResponseBody(response, false, -2, "Invalid schedule item.");
                    }

                    scheduleList.add(AppConfig::RecordingPlan{
                        .startTimestamp = static_cast<int64_t>(cJSON_GetNumberValue(startTimestamp)),
                        .duration       = static_cast<uint32_t>(cJSON_GetNumberValue(duration)),
                    });
                }

                xSemaphoreTake(globalAppMutex, portMAX_DELAY);
                globalAppConfig.value.recording.schedule = std::move(scheduleList);
                globalAppConfig.markUpdated();
                globalAppConfig.value.save(appConfigFileName);
                xSemaphoreGive(globalAppMutex);

                return HttpServiceUtil::sendResponseBody(response, true, 0, "Recording schedule successfully updated.");
            }

            HttpServiceUtil::sendResponseBody(response, false, -3, "Missing array parameter: `schedule`.");
        }
    };
} // namespace

auto&& updateScheduleService = []() -> HttpService& {
    static UpdateScheduleService service;

    return service;
}();
