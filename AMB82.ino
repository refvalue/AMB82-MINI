#undef min
#undef max
#undef true
#undef false

#include "AppConfig.hpp"
#include "BleServer.hpp"
#include "DS3231.hpp"
#include "DateTime.hpp"
#include "HttpServer.hpp"
#include "MixingStreamer.hpp"
#include "RecordingStateMachine.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"
#include "TrackedValue.hpp"
#include "WiFiHotspot.hpp"

#include <atomic>
#include <cstdint>
#include <ctime>
#include <memory>
#include <tuple>
#include <utility>

#include <AmebaFatFS.h>
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
    constexpr int32_t videoChannel = 0;

    std::shared_ptr<TrackedValue<AppConfig>::ElementPair> appConfigCache;

    DS3231 rtc{Wire};
    VideoSetting videoSetting{videoChannel};
    MixingStreamer streamer;
    RecordingStateMachine recordingStateMachine;

    HttpServer webServer{80};
    HttpServer liveStreamingServer{8080};

    BleServer bleServer{"NinoCam Smart Box", "4fafc201-1fb5-459e-8fcc-c5c9c331914b",
        "beb5483e-36e1-4688-b7f5-ea07361b26a8", "d506d318-2fbc-4d2c-8a67-f14b7313f3df"};

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

    void driveRecording(int64_t timestamp, const String& dateTime) {
        static bool recorded;
        auto&& [config, updated] = *appConfigCache;

        if (updated) {
            recordingStateMachine.update(config.recording.schedule);
        }

        const auto baseName           = config.recording.baseName;
        const auto singleFileDuration = config.recording.singleFileDuration;
        const auto item               = recordingStateMachine.tryMatch(timestamp, recorded);

        if (item) {
            streamer.setBaseFileName(baseName);
            streamer.setSingleFileDuration(singleFileDuration);
            streamer.startRecording(timestamp);

            Serial.print("Start recording: ");
            Serial.println(dateTime);
            Serial.print("Duration: ");
            Serial.println(item->duration);
        } else if (recorded) {
            streamer.tick(timestamp);
        } else if (streamer.stopRecording(timestamp)) {
            Serial.print("Stop recording: ");
            Serial.println(dateTime);
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
} // namespace

QueueHandle_t globalAppMutex;
DS3231& globalRtc = rtc;

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        // Waits for serial port to connect. Needed for native USB port only。
    }

    rtc.begin();
    loadConfig();
    initMultimedia();

    if (rtc.alarm1Triggered()) {
        rtc.clearAlarm1Flag();
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
    static time_t unixTimestamp;
    static DateTime dateTime;

    xSemaphoreTake(globalAppMutex, portMAX_DELAY);
    dateTime      = rtc.getDateTime();
    unixTimestamp = TimeUtil::toUnixTimestamp(dateTime);
    xSemaphoreGive(globalAppMutex);

    const auto dateTimeText = ctime(&unixTimestamp);

    OSD.createBitmap(videoChannel);
    OSD.drawText(videoChannel, 36, 36, dateTimeText, 0xFFFFFFFF);
    OSD.update(videoChannel);

    updateConfigCache();
    updateWiFiHotspot();
    driveRecording(unixTimestamp, dateTimeText);

    if (appConfigCache->second) {
        globalAppConfig.confirm();
    }

    if (++counter % 10 == 0) {
        WiFiHotspot.dump();
        Serial.println(dateTimeText);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
}
