#include "AppConfig.hpp"

#include "Resources.hpp"
#include "TlvReader.hpp"
#include "TlvWriter.hpp"

#include <utility>

#include <LOGUARTClass.h>

namespace {
    constexpr char defaultSSID[]                 = "AMB82-MINI";
    constexpr char defaultPassword[]             = "12345678";
    constexpr char defaultBaseName[]             = "recording";
    constexpr uint32_t defaultSingleFileDuration = 60 * 30;
} // namespace

void AppConfig::saveToFlash() {}

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

void AppConfig::dump() {}

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
    return createDefault();
}

AppConfig AppConfig::fromBuffer(std::span<const uint8_t> buffer) {
    TlvReader reader{buffer};
    auto config = createDefault();

    reader.addType(1,
        TlvReader::DataHandler<uint8_t>{[&](uint8_t type, uint8_t value) { config.hotspot.enabled = (value != 0); }});

    reader.addType(
        2, TlvReader::DataHandler<String>{[&](uint8_t type, String value) { config.hotspot.ssid = std::move(value); }});

    reader.addType(3, TlvReader::DataHandler<String>{
                          [&](uint8_t type, String value) { config.hotspot.password = std::move(value); }});

    reader.addType(4, TlvReader::DataHandler<String>{
                          [&](uint8_t type, String value) { config.recording.baseName = std::move(value); }});

    reader.addType(5, TlvReader::DataHandler<uint32_t>{
                          [&](uint8_t type, uint32_t value) { config.recording.singleFileDuration = value; }});

    for (size_t i = 0; i < 8; i++) {
        reader.addType(
            static_cast<uint8_t>(100 + i * 2), TlvReader::DataHandler<uint64_t>{[&, i](uint8_t type, uint64_t value) {
                config.recording.schedule.resize(i + 1);
                config.recording.schedule[i].startTimestamp = static_cast<int64_t>(value);
            }});

        reader.addType(
            static_cast<uint8_t>(101 + i * 2), TlvReader::DataHandler<uint32_t>{[&, i](uint8_t type, uint32_t value) {
                config.recording.schedule.resize(i + 1);
                config.recording.schedule[i].duration = value;
            }});
    }

    reader.readAll();

    return config;
}

TrackedValue<AppConfig> globalAppConfig{AppConfig::createDefault()};
