#include "BleService.hpp"
#include "DS3231.hpp"
#include "MessageUtil.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "cJSON.hpp"

#include <cstdint>
#include <ctime>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>
#include <task.h>

namespace {
    struct UpdateTimeService : BleService {
        void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) override {
            if (cJSON_IsNull(message) || cJSON_IsInvalid(message)) {
                return MessageUtil::sendResponseBody(transport, "failure", -1, "Invalid request body.");
            }

            if (const auto item = cJSON_GetObjectItem(message, "timestamp"); item && cJSON_IsNumber(item)) {
                const auto timestamp = cJSON_GetNumberValue(item);

                xSemaphoreTake(globalAppMutex, portMAX_DELAY);
                globalRtc.setDateTime(TimeUtil::fromUnixTimestamp(static_cast<int64_t>(timestamp)));
                xSemaphoreGive(globalAppMutex);

                return MessageUtil::sendResponseBody(transport, true, 0, "RTC time successfully synchornized.");
            }

            MessageUtil::sendResponseBody(transport, false, -2, "Missing numeric parameter: `timestamp`.");
        }
    };
} // namespace

auto&& updateTimeService = []() -> BleService& {
    static UpdateTimeService service;

    return service;
}();
