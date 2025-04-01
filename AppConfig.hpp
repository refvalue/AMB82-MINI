#pragma once

#include "SafeList.hpp"
#include "cJSON.hpp"

#include <WString.h>
#include <stdint.h>

struct AppConfig {
    struct HotspotConfig {
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
        SafeList<RecordingPlan> schedule;
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
