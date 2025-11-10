#include "AppConfig.hpp"
#include "BleService.hpp"
#include "Resources.hpp"

#include <cstdint>
#include <span>
#include <utility>

#if 1
#include <FreeRTOS.h>
#endif

#include <LOGUARTClass.h>
#include <semphr.h>

namespace {
    class UpdateScheduleService : public BleService {
    public:
        void run(uint8_t type, std::span<const uint8_t> data, SendHandler sendHandler) override {
            for (size_t i = 0; i < data.size(); i++) {
                Serial.print(data[i]);
                Serial.print("=");
            }

            Serial.println();

            auto tmp = AppConfig::fromBuffer(data);

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            auto config = globalAppConfig.current()->first;
            xSemaphoreGive(globalAppMutex);

            config.recording.schedule = std::move(tmp.recording.schedule);
            globalAppConfig.update(std::move(config));

            sendHandler(std::array<uint8_t, 2>{'O', 'K'});
        }
    };
} // namespace

auto&& updateScheduleService = []() -> BleService& {
    static UpdateScheduleService service;

    return service;
}();
