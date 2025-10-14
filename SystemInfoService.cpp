#include "AppConfig.hpp"
#include "DS3234.hpp"
#include "HttpMessageServer.hpp"
#include "HttpService.hpp"
#include "HttpServiceUtil.hpp"
#include "Resources.hpp"
#include "cJSON.hpp"

#include <AmebaFatFS.h>
#include <Client.h>
#include <WString.h>
#include <WiFi.h>

namespace {
    struct SystemInfoService : HttpService {
        void run(const String& methodPath, HttpMessage& message, Client& client) override {
            HttpMessageServer response{client};
            cJSONPtr data{cJSON_CreateObject()};

            const auto sdcard = cJSON_AddObjectToObject(data.get(), "sdcard");

            cJSON_AddNumberToObject(sdcard, "freeSpace", static_cast<double>(SDFs.get_free_space()));
            cJSON_AddNumberToObject(sdcard, "usedSpace", static_cast<double>(SDFs.get_used_space()));

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            cJSON_AddNumberToObject(data.get(), "timestamp", getDS3234().getDateTime().Unix64Time());
            cJSON_AddItemToObject(data.get(), "hotspot", globalAppConfig.value.getHotspotJson().detach());
            xSemaphoreGive(globalAppMutex);

            HttpServiceUtil::sendResponseBody(response, true, 0, "Success", data.get());
        }
    };
} // namespace

auto&& systemInfoService = []() -> HttpService& {
    static SystemInfoService service;

    return service;
}();
