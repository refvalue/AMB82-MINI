#pragma once

#include "TrackedValue.hpp"

#include <cstdint>
#include <span>
#include <vector>

#include <WString.h>

class TlvWriter;

struct AppConfig {
    struct HotspotConfig {
        bool enabled{};
        String ssid;
        String password;
    };

    struct RecordingPlan {
        int64_t startTimestamp{};
        uint32_t duration{};
    };

    struct RecordingConfig {
        String baseName;
        uint32_t singleFileDuration{};
        std::vector<RecordingPlan> schedule;
    };

    HotspotConfig hotspot;
    RecordingConfig recording;

    void saveToFlash();
    void writeTlv(TlvWriter& writer) const;
    void dump();

    static AppConfig createDefault();
    static AppConfig fromFlash();
    static AppConfig fromBuffer(std::span<const uint8_t> buffer);
};

extern TrackedValue<AppConfig> globalAppConfig;
