#include "AppConfig.hpp"

#include "Resources.hpp"
#include "TlvReader.hpp"
#include "TlvWriter.hpp"

#include <algorithm>
#include <utility>

#include <FlashMemory.h>
#include <LOGUARTClass.h>

namespace {
    constexpr char defaultSSID[]                 = "AMB82-MINI";
    constexpr char defaultPassword[]             = "12345678";
    constexpr char defaultBaseName[]             = "recording";
    constexpr uint32_t defaultSingleFileDuration = 60 * 30;
    constexpr uint32_t maxSingleFileDuration     = 2048;
} // namespace

void AppConfig::saveToFlash() {
    TlvWriter writer;

    writeTlv(writer);
    FlashMemory.read();
    std::ranges::copy(writer.data(), FlashMemory.buf);
    FlashMemory.write();

    Serial.println("AppConfig saved to flash.");
}

void AppConfig::writeTlv(TlvWriter& writer) const {
    writer.write(1, static_cast<uint8_t>(hotspot.enabled ? 1 : 0));
    writer.write(2, hotspot.ssid, 12);
    writer.write(3, hotspot.password, 8);

    writer.write(4, recording.baseName, 12);
    writer.write(5, recording.singleFileDuration);

    const auto scheduleCount = std::min(recording.schedule.size(), 8U);

    for (size_t i = 0; i < scheduleCount; i++) {
        auto&& item = recording.schedule[i];

        writer.write(static_cast<uint8_t>(100 + i * 2), static_cast<uint64_t>(item.startTimestamp));
        writer.write(static_cast<uint8_t>(101 + i * 2), item.duration);
    }
}

void AppConfig::dump() {
    Serial.println("AppConfig Dump:");
    Serial.print("  Hotspot Enabled: ");
    Serial.println(hotspot.enabled ? "Yes" : "No");
    Serial.print("  Hotspot SSID: ");
    Serial.println(hotspot.ssid);
    Serial.print("  Hotspot Password: ");
    Serial.println(hotspot.password);
    Serial.print("  Recording Base Name: ");
    Serial.println(recording.baseName);
    Serial.print("  Recording Single File Duration: ");
    Serial.println(recording.singleFileDuration);
    Serial.println("  Recording Schedule:");

    for (size_t i = 0; i < recording.schedule.size(); i++) {
        auto&& item = recording.schedule[i];
        Serial.print("    [");
        Serial.print(i);
        Serial.print("] Start Timestamp: ");
        Serial.print(item.startTimestamp);
        Serial.print(", Duration: ");
        Serial.println(item.duration);
    }
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

AppConfig AppConfig::fromFlash() {
    FlashMemory.read();
    Serial.println("AppConfig loaded from flash.");

    return fromBuffer({FlashMemory.buf, FlashMemory.buf_size});
}

AppConfig AppConfig::fromBuffer(std::span<const uint8_t> buffer) {
    TlvReader reader{buffer};
    auto config = createDefault();

    reader.registerHandler(1,
        TlvReader::DataHandler<uint8_t>{[&](uint8_t type, uint8_t value) { config.hotspot.enabled = (value != 0); }});

    reader.registerHandler(
        2, TlvReader::DataHandler<String>{[&](uint8_t type, String value) { config.hotspot.ssid = std::move(value); }});

    reader.registerHandler(3, TlvReader::DataHandler<String>{
                                  [&](uint8_t type, String value) { config.hotspot.password = std::move(value); }});

    reader.registerHandler(4, TlvReader::DataHandler<String>{
                                  [&](uint8_t type, String value) { config.recording.baseName = std::move(value); }});

    reader.registerHandler(5, TlvReader::DataHandler<uint32_t>{[&](uint8_t type, uint32_t value) {
        if (value > maxSingleFileDuration) {
            value = defaultSingleFileDuration;
            Serial.println("Found invalid `SingleFileDuration`, using the recommended value ");
            Serial.print(defaultSingleFileDuration);
            Serial.println(".");
        }

        config.recording.singleFileDuration = value;
    }});

    for (size_t i = 0; i < 8; i++) {
        reader.registerHandler(
            static_cast<uint8_t>(100 + i * 2), TlvReader::DataHandler<uint64_t>{[&, i](uint8_t type, uint64_t value) {
                config.recording.schedule.resize(i + 1);
                config.recording.schedule[i].startTimestamp = static_cast<int64_t>(value);
                Serial.println(static_cast<int64_t>(value));
            }});

        reader.registerHandler(
            static_cast<uint8_t>(101 + i * 2), TlvReader::DataHandler<uint32_t>{[&, i](uint8_t type, uint32_t value) {
                config.recording.schedule.resize(i + 1);
                config.recording.schedule[i].duration = value;
                Serial.println(value);
            }});
    }

    if (reader.readAll()) {
        Serial.println("AppConfig parsed from buffer.");
    } else {
        Serial.println("AppConfig parsing from buffer failed, using defaults where necessary.");
        config = createDefault();
    }

    return config;
}

TrackedValue<AppConfig> globalAppConfig{AppConfig::createDefault()};
