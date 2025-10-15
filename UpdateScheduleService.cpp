#include "AppConfig.hpp"
#include "BleService.hpp"
#include "MessageUtil.hpp"
#include "Resources.hpp"
#include "SafeList.hpp"
#include "cJSON.hpp"

#include <cstdint>
#include <utility>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    struct UpdateScheduleService : BleService {
        void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) override {
            if (cJSON_IsNull(message) || cJSON_IsInvalid(message)) {
                return MessageUtil::sendResponseBody(transport, "failure", -1, "Invalid request body.");
            }

            if (const auto schedule = cJSON_GetObjectItem(message, "schedule"); schedule && cJSON_IsArray(schedule)) {
                SafeList<AppConfig::RecordingPlan> scheduleList;
                cJSON* startTimestamp{};
                cJSON* duration{};

                for (auto item = schedule->child; item; item = item->next) {
                    if (!cJSON_IsObject(item)
                        || (startTimestamp = cJSON_GetObjectItem(item, "startTimestamp")) == nullptr
                        || (duration = cJSON_GetObjectItem(item, "duration")) == nullptr
                        || !cJSON_IsNumber(startTimestamp) || !cJSON_IsNumber(duration)) {
                        return MessageUtil::sendResponseBody(transport, false, -2, "Invalid schedule item.");
                    }

                    scheduleList.add(AppConfig::RecordingPlan{
                        .startTimestamp = static_cast<int64_t>(cJSON_GetNumberValue(startTimestamp)),
                        .duration       = static_cast<uint32_t>(cJSON_GetNumberValue(duration)),
                    });
                }

                xSemaphoreTake(globalAppMutex, portMAX_DELAY);
                globalAppConfig.value.recording.schedule = std::move(scheduleList);
                globalAppConfig.markUpdated();
                xSemaphoreGive(globalAppMutex);

                return MessageUtil::sendResponseBody(transport, true, 0, "Recording schedule successfully updated.");
            }

            MessageUtil::sendResponseBody(transport, false, -3, "Missing array parameter: `schedule`.");
        }
    };
} // namespace

auto&& updateScheduleService = []() -> BleService& {
    static UpdateScheduleService service;

    return service;
}();
