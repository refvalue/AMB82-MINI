#include "BleService.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "TlvReader.hpp"
#include "TlvWriter.hpp"

#include <cstdint>
#include <span>

namespace {
    class UpdateTimeService : public BleService {
    public:
        void run(uint8_t type, std::span<const uint8_t> data, Btp::BtpTransport& transport) override {
            TlvReader reader{data};

            reader.addType(1, TlvReader::DataHandler<uint64_t>{
                                  [&](uint8_t type, uint64_t value) {
                                      const auto timestamp = static_cast<int64_t>(value);

                                      globalPendingTimestampSince2020.store(
                                          TimeUtil::toTimestampSince2020(timestamp), std::memory_order_release);
                                  },
                              });

            reader.readAll();
        }
    };
} // namespace

auto&& updateTimeService = []() -> BleService& {
    static UpdateTimeService service;

    return service;
}();
