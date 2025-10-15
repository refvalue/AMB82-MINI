#include "AppConfig.hpp"
#include "BleService.hpp"
#include "MessageUtil.hpp"
#include "Resources.hpp"
#include "cJSON.hpp"

#include <cstddef>
#include <cstdint>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    struct CurrentScheduleService : BleService {
        void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) override {
            cJSONPtr data{cJSON_CreateObject()};

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            const auto schedule = globalAppConfig.value.getScheduleJson();
            xSemaphoreGive(globalAppMutex);

            cJSON_AddItemReferenceToObject(data.get(), "schedule", schedule.get());
            MessageUtil::sendResponseBody(transport, true, 0, "Success", data.get());
        }
    };
} // namespace

auto&& currentScheduleService = []() -> BleService& {
    static CurrentScheduleService service;

    return service;
}();
