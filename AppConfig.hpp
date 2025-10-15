#pragma once

#include "TrackedValue.hpp"
#include "cJSON.hpp"

#include <cstdint>
#include <vector>

#include <WString.h>

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

    void save(const String& path);
    void dump();
    cJSONPtr getHotspotJson();
    cJSONPtr getScheduleJson();

    static AppConfig createDefault();
    static AppConfig fromFile(const String& path);
};

extern TrackedValue<AppConfig> globalAppConfig;
