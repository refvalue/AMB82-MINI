#include "AppConfig.hpp"
#include "BleService.hpp"
#include "Resources.hpp"
#include "TlvWriter.hpp"

#include <cstddef>
#include <cstdint>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    class CurrentScheduleService : public BleService {
    public:
        void run(uint8_t type, std::span<const uint8_t> data, Btp::BtpTransport& transport) override {
            xSemaphoreTake(globalAppMutex, portMAX_DELAY);
            const auto config = globalAppConfig.current()->first;
            xSemaphoreGive(globalAppMutex);

            writer_.clear();
            config.writeTlv(writer_);
        }

    private:
        TlvWriter writer_;
    };
} // namespace

auto&& currentScheduleService = []() -> BleService& {
    static CurrentScheduleService service;

    return service;
}();
