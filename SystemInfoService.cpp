#include "AppConfig.hpp"
#include "BleService.hpp"
#include "DS3231.hpp"
#include "MessageUtil.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "cJSON.hpp"

#include <cstdint>

#include <AmebaFatFS.h>
#include <WiFi.h>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    struct SystemInfoService : BleService {
        void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) override {
            cJSONPtr data{cJSON_CreateObject()};

            const auto sdcard = cJSON_AddObjectToObject(data.get(), "sdcard");

            cJSON_AddNumberToObject(sdcard, "freeSpace", static_cast<double>(SDFs.get_free_space()));
            cJSON_AddNumberToObject(sdcard, "usedSpace", static_cast<double>(SDFs.get_used_space()));

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            cJSON_AddNumberToObject(data.get(), "timestamp", TimeUtil::toUnixTimestamp(globalRtc.getDateTime()));
            cJSON_AddItemToObject(data.get(), "hotspot", globalAppConfig.current()->first.getHotspotJson().detach());
            xSemaphoreGive(globalAppMutex);

            MessageUtil::sendResponseBody(transport, true, 0, "Success", data.get());
        }
    };
} // namespace

auto&& systemInfoService = []() -> BleService& {
    static SystemInfoService service;

    return service;
}();
