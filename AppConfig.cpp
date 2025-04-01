#include "AppConfig.hpp"
#include "Resources.hpp"
#include "cJSON.h"

#include <AmebaFatFS.h>
#include <Arduino.h>
#include <Optional.h>

namespace {
    constexpr char defaultSSID[]                 = "AMB82 Mini Box";
    constexpr char defaultPassword[]             = "12345678";
    constexpr char defaultBaseName[]             = "recording";
    constexpr uint32_t defaultSingleFileDuration = 60 * 30;

    AppConfig::HotspotConfig parseHotspotConfig(cJSON* json) {
        auto hotspot = cJSON_GetObjectItem(json, "hotspot");

        if (hotspot == nullptr || !cJSON_IsObject(hotspot)) {
            hotspot = cJSON_AddObjectToObject(json, "hotspot");
        }

        auto ssid = cJSON_GetObjectItem(hotspot, "ssid");

        if (ssid == nullptr || !cJSON_IsString(ssid)) {
            cJSON_AddStringToObject(hotspot, "ssid", defaultSSID);
        }

        auto password = cJSON_GetObjectItem(hotspot, "password");

        if (password == nullptr || !cJSON_IsString(password)) {
            cJSON_AddStringToObject(hotspot, "password", defaultPassword);
        }

        AppConfig::HotspotConfig config{
            .ssid     = cJSON_GetStringValue(ssid),
            .password = cJSON_GetStringValue(password),
        };

        if (config.ssid.length() < 1 || config.ssid.length() > 32) {
            Serial.println("Invalid SSID. The length must be in range of 1-32.");
            Serial.println("Roll back to the default SSID.");
            config.ssid = defaultSSID;
        }

        if (config.password.length() < 8 || config.password.length() > 63) {
            Serial.println("Invalid Wi-Fi password. The length must be in range of 8-63.");
            Serial.println("Roll back to the default password.");
            config.password = defaultPassword;
        }

        return config;
    }

    Optional<AppConfig::RecordingPlan> parseRecordingPlan(const cJSON* json) {
        Optional<AppConfig::RecordingPlan> result;
        const auto startTimestamp = cJSON_GetObjectItem(json, "startTimestamp");

        if (startTimestamp == nullptr || !cJSON_IsNumber(startTimestamp)) {
            return result;
        }

        const auto duration = cJSON_GetObjectItem(json, "duration");

        if (duration == nullptr || !cJSON_IsNumber(duration)) {
            return result;
        }

        result.emplace();
        result.ref().startTimestamp = static_cast<int64_t>(cJSON_GetNumberValue(startTimestamp));
        result.ref().duration       = static_cast<uint32_t>(cJSON_GetNumberValue(duration));

        if (result.ref().duration < 1) {
            result.ref().duration = 1;
        }

        return result;
    }

    AppConfig::RecordingConfig parseRecordingConfig(cJSON* json) {
        auto recording = cJSON_GetObjectItem(json, "recording");

        if (recording == nullptr || !cJSON_IsObject(recording)) {
            recording = cJSON_AddObjectToObject(json, "recording");
        }

        auto baseName = cJSON_GetObjectItem(recording, "baseName");

        if (baseName == nullptr || !cJSON_IsString(baseName)) {
            baseName = cJSON_AddStringToObject(recording, "baseName", defaultBaseName);
        }

        auto singleFileDuration = cJSON_GetObjectItem(recording, "singleFileDuration");

        if (singleFileDuration == nullptr || !cJSON_IsNumber(singleFileDuration)) {
            singleFileDuration = cJSON_AddNumberToObject(
                recording, "singleFileDuration", static_cast<double>(defaultSingleFileDuration));
        }

        auto schedule = cJSON_GetObjectItem(recording, "schedule");

        if (schedule == nullptr || !cJSON_IsArray(schedule)) {
            schedule = cJSON_AddArrayToObject(recording, "schedule");
        }

        AppConfig::RecordingConfig config{
            .baseName           = cJSON_GetStringValue(baseName),
            .singleFileDuration = static_cast<uint32_t>(cJSON_GetNumberValue(singleFileDuration)),
        };

        if (config.baseName.length() < 1) {
            Serial.println("The recording base name must be non-empty.");
            Serial.println("Roll back to the default base name.");
            config.baseName = defaultBaseName;
        }

        if (config.singleFileDuration == 0) {
            Serial.println("The recording single file duration must be non-zero.");
            Serial.println("Roll back to the default single file duration.");
            config.singleFileDuration = defaultSingleFileDuration;
        }

        for (auto item = schedule->child; item; item = item->next) {
            if (auto plan = parseRecordingPlan(item)) {
                config.schedule.add(plan.getValue());
            } else {
                Serial.println("Invalid schedule item detected.");
            }
        }

        return config;
    }

    cJSONPtr makeJson(AppConfig& config) {
        cJSONPtr json{cJSON_CreateObject()};

        cJSON_AddItemToObject(json.get(), "hotspot", config.getHotspotJson().detach());

        const auto recording = cJSON_AddObjectToObject(json.get(), "recording");

        cJSON_AddStringToObject(recording, "baseName", config.recording.baseName.c_str());
        cJSON_AddNumberToObject(
            recording, "singleFileDuration", static_cast<double>(config.recording.singleFileDuration));
        cJSON_AddItemToObject(recording, "schedule", config.getScheduleJson().detach());

        return json;
    }
} // namespace

void AppConfig::save(const String& path) {
    if (SDFs.exists(path)) {
        SDFs.remove(path);
    }

    if (auto file = SDFs.open(path)) {
        if (const auto str = cJSON_Print(makeJson(*this).get())) {
            file.write(str);
            Serial.println(str);
            Serial.println("New configurations have been saved to the SD card.");
            cJSON_free(str);
        }

        file.close();
    }
}

void AppConfig::dump() {
    if (const auto str = cJSON_Print(makeJson(*this).get())) {
        Serial.println(str);
        cJSON_free(str);
    }
}

cJSONPtr AppConfig::getHotspotJson() {
    cJSONPtr hotspot{cJSON_CreateObject()};

    cJSON_AddStringToObject(hotspot.get(), "ssid", this->hotspot.ssid.c_str());
    cJSON_AddStringToObject(hotspot.get(), "password", this->hotspot.password.c_str());

    return hotspot;
}

cJSONPtr AppConfig::getScheduleJson() {
    cJSONPtr schedule{cJSON_CreateArray()};

    for (size_t i = 0; i < static_cast<size_t>(recording.schedule.getSize()); i++) {
        const auto item = cJSON_CreateObject();
        auto&& plan     = recording.schedule.at(i);

        cJSON_AddItemToArray(schedule.get(), item);
        cJSON_AddNumberToObject(item, "startTimestamp", static_cast<double>(plan.startTimestamp));
        cJSON_AddNumberToObject(item, "duration", static_cast<double>(plan.duration));
    }

    return schedule;
}

AppConfig AppConfig::createDefault() {
    return {
        .hotspot =
            {
                .ssid     = defaultSSID,
                .password = defaultPassword,
            },
        .recording =
            {
                .baseName           = defaultBaseName,
                .singleFileDuration = defaultSingleFileDuration,
            },
    };
}

AppConfig AppConfig::fromFile(const String& path) {
    if (SDFs.exists(path)) {
        if (auto file = SDFs.open(path)) {
            const auto content = file.readString();

            file.close();

            if (cJSONPtr json{cJSON_ParseWithLength(content.c_str(), content.length())};
                json && !cJSON_IsInvalid(json.get()) && cJSON_IsObject(json.get())) {
                return {
                    .hotspot   = parseHotspotConfig(json.get()),
                    .recording = parseRecordingConfig(json.get()),
                };
            }
        }
    }

    return createDefault();
}
