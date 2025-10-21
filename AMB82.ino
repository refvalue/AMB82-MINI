#include "AppConfig.hpp"
#include "BleServer.hpp"
#include "DS3231.hpp"
#include "DateTime.hpp"
#include "HttpServer.hpp"
#include "MixingStreamer.hpp"
#include "RecordingController.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "TrackedValue.hpp"
#include "WiFiHotspot.hpp"

#include <atomic>
#include <cstdint>
#include <limits>
#include <memory>
#include <tuple>
#include <utility>

#include <AmebaFatFS.h>
#include <BLEDevice.h>
#include <VideoStream.h>
#include <VideoStreamOverlay.h>
#include <Wire.h>

extern BleService& systemInfoService;
extern BleService& currentScheduleService;
extern BleService& updateTimeService;
extern BleService& updateScheduleService;
extern BleService& configureWiFiHotspotService;

extern HttpService& fallbackService;
extern HttpService& videoStreamingService;

extern MMFModule& videoStreamingMMFModule;

namespace {
    constexpr auto noPendingValue = std::numeric_limits<int32_t>::min();
}

std::atomic_int32_t globalNowSince2020{0};
std::atomic_int32_t globalPendingTimestampSince2020{noPendingValue};

QueueHandle_t globalAppMutex;

namespace {
    constexpr int32_t videoChannel = 0;
    constexpr char deviceName[]    = "NINOCAM";
    constexpr char serviceUuid[]   = "4fafc201-1fb5-459e-8fcc-c5c9c331914b";
    constexpr char rxUuid[]        = "beb5483e-36e1-4688-b7f5-ea07361b26a8";
    constexpr char txUuid[]        = "d506d318-2fbc-4d2c-8a67-f14b7313f3df";

    std::shared_ptr<TrackedValue<AppConfig>::ElementPair> appConfigCache;

    DS3231 ds3231{Wire};
    VideoSetting videoSetting{videoChannel};
    MixingStreamer streamer;
    RecordingController recordingController{ds3231, streamer};

    HttpServer webServer{80};
    HttpServer liveStreamingServer{8080};
    BleServer bleServer{deviceName, serviceUuid, rxUuid, txUuid};

    void initMultimedia() {
        Camera.configVideoChannel(videoChannel, videoSetting);
        Camera.videoInit(0);

        streamer.init(Camera.getStream(videoChannel), videoStreamingMMFModule, videoSetting);

        // Configures the video overlay system.
        OSD.configVideo(videoChannel, videoSetting);
        OSD.configTextSize(videoChannel, 30, 48);
        OSD.begin();
    }

    void loadConfig() {
        SDFs.begin();
        globalAppMutex = xSemaphoreCreateMutex();
        globalAppConfig.update(AppConfig::fromFile(appConfigFileName));
        globalAppConfig.current()->first.dump();
    }

    void updateConfigCache() {
        auto cache               = globalAppConfig.current();
        auto&& [config, updated] = *cache;

        if (updated) {
            config.save(appConfigFileName);
        }

        appConfigCache = std::move(cache);
    }

    void updateDateTime() {
        if (const auto timestamp = globalPendingTimestampSince2020.exchange(noPendingValue, std::memory_order_acq_rel);
            timestamp != noPendingValue) {
            ds3231.setDateTime(TimeUtil::toDateTimeFromSince2020(timestamp));
            Serial.print("System time updated: ");
            Serial.println(TimeUtil::toIso8601(ds3231.getDateTime()));
        }
    }

    void updateWiFiHotspot() {
        auto&& [config, updated] = *appConfigCache;

        if (updated) {
            if (config.hotspot.enabled) {
                WiFiHotspot.start(config.hotspot.ssid, config.hotspot.password, 1);
                webServer.start();
                liveStreamingServer.start();
                Serial.println("WiFi Hotspot started.");
            } else {
                webServer.stop();
                liveStreamingServer.stop();
                WiFiHotspot.stop();
                Serial.println("WiFi Hotspot stopped.");
            }
        }
    }

    void driveRecording() {
        auto&& [config, updated] = *appConfigCache;

        if (updated) {
            recordingController.update(config.recording);
        }

        recordingController.tick();
    }
} // namespace

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        // Waits for serial port to connect. Needed for native USB port only。
    }

    Serial.print("System free heap size: ");
    Serial.println(xPortGetFreeHeapSize());

    ds3231.begin();
    loadConfig();
    initMultimedia();

    if (ds3231.alarm1Triggered()) {
        ds3231.clearAlarm1Flag();
    }

    webServer.setFallbackService(&fallbackService);
    liveStreamingServer.addService("GET /live", &videoStreamingService);

    bleServer.addService(RequestType::getSystemInfo, &systemInfoService);
    bleServer.addService(RequestType::getRecordingSchedule, &currentScheduleService);
    bleServer.addService(RequestType::setSystemTime, &updateTimeService);
    bleServer.addService(RequestType::setRecordingSchedule, &updateScheduleService);
    bleServer.addService(RequestType::configureWiFiHotspot, &configureWiFiHotspotService);
    bleServer.start();
    
    // Starts feeding multimedia data.
    Camera.channelBegin(videoChannel);
}

void loop() {
    static int32_t counter;

    const auto dateTime     = ds3231.getDateTime();
    const auto dateTimeText = TimeUtil::toIso8601(dateTime);

    globalNowSince2020.store(TimeUtil::toTimestampSince2020(dateTime), std::memory_order::release);

    OSD.createBitmap(videoChannel);
    OSD.drawText(videoChannel, 36, 36, dateTimeText.c_str(), 0xFFFFFFFF);
    OSD.update(videoChannel);

    updateDateTime();
    updateConfigCache();
    //updateWiFiHotspot();
    driveRecording();

    if (appConfigCache->second) {
        globalAppConfig.confirm();
    }

    if (++counter % 2 == 0) {
        Serial.println(dateTimeText);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
}
