#include "BleService.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "TlvWriter.hpp"

#include <cstdint>

#include <AmebaFatFS.h>
#include <WiFi.h>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

namespace {
    class SystemInfoService : public BleService {
    public:
        void run(uint8_t type, std::span<const uint8_t> data, Btp::BtpTransport& transport) override {
            writer_.clear();
            writer_.write(1, static_cast<uint32_t>(SDFs.get_free_space()));
            writer_.write(2, static_cast<uint32_t>(SDFs.get_used_space()));

            const auto timestamp =
                TimeUtil::toUnixTimestampFromSince2020(globalNowSince2020.load(std::memory_order_acquire));

            writer_.write(3, static_cast<uint64_t>(timestamp));
            transport.send(writer_.data());
        }

    private:
        TlvWriter writer_;
    };
} // namespace

auto&& systemInfoService = []() -> BleService& {
    static SystemInfoService service;

    return service;
}();
