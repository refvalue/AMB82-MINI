#include "AppConfig.hpp"
#include "BleService.hpp"
#include "MessageUtil.hpp"
#include "Resources.hpp"
#include "cJSON.hpp"

#include <cstdint>
#include <utility>
#include <vector>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    struct ConfigureWiFiHotspotService : BleService {
        void run(int32_t type, cJSON* message, Btp::BtpTransport& transport) override {
            if (cJSON_IsNull(message) || cJSON_IsInvalid(message)) {
                return MessageUtil::sendResponseBody(transport, "failure", -1, "Invalid request body.");
            }

            if (const auto enabled = cJSON_GetObjectItem(message, "enabled"); enabled && cJSON_IsBool(enabled)) {
                xSemaphoreTake(globalAppMutex, portMAX_DELAY);
                auto config            = globalAppConfig.current()->first;
                config.hotspot.enabled = cJSON_IsTrue(enabled);
                globalAppConfig.update(std::move(config));
                xSemaphoreGive(globalAppMutex);

                return MessageUtil::sendResponseBody(transport, true, 0, "Hotspot configuration successfully updated.");
            }

            MessageUtil::sendResponseBody(transport, false, -3, "Missing boolean parameter: `enabled`.");
        }
    };
} // namespace

auto&& configureWiFiHotspotService = []() -> BleService& {
    static ConfigureWiFiHotspotService service;

    return service;
}();
