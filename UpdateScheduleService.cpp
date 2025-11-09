#include "AppConfig.hpp"
#include "BleService.hpp"
#include "Resources.hpp"

#include <cstdint>
#include <span>
#include <utility>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    class UpdateScheduleService : public BleService {
    public:
        void run(uint8_t type, std::span<const uint8_t> data, Btp::BtpTransport& transport) override {
            auto tmp = AppConfig::fromBuffer(data);

            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            auto config = globalAppConfig.current()->first;
            xSemaphoreGive(globalAppMutex);

            config.recording.schedule = std::move(tmp.recording.schedule);
            globalAppConfig.update(std::move(config));
        }
    };
} // namespace

auto&& updateScheduleService = []() -> BleService& {
    static UpdateScheduleService service;

    return service;
}();
