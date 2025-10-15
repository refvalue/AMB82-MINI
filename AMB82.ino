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

#include <cstdint>
#include <ctime>
#include <tuple>

#include <AmebaFatFS.h>
#include <VideoStream.h>
#include <VideoStreamOverlay.h>
#include <Wire.h>

extern BleService& systemInfoService;
extern BleService& currentScheduleService;
extern BleService& updateTimeService;
extern BleService& updateScheduleService;

extern HttpService& fallbackService;
extern HttpService& videoStreamingService;

extern MMFModule& videoStreamingMMFModule;

namespace {
    constexpr int32_t videoChannel = 0;

    TrackedValue<AppConfig> appConfig{AppConfig::createDefault()};

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

    void loadAppConfig() {
        SDFs.begin();
        globalAppMutex = xSemaphoreCreateMutex();
        appConfig.update(AppConfig::fromFile(appConfigFileName));
        appConfig.value.dump();
    }

    void driveRecording(int64_t timestamp, const String& dateTime) {
        static bool recorded;

        xSemaphoreTake(globalAppMutex, portMAX_DELAY);
        if (appConfig.updated) {
            appConfig.confirm();
            appConfig.value.save(appConfigFileName);
            recordingStateMachine.update(appConfig.value.recording.schedule);
        }

        const auto baseName           = appConfig.value.recording.baseName;
        const auto singleFileDuration = appConfig.value.recording.singleFileDuration;
        const auto item               = recordingStateMachine.tryMatch(timestamp, recorded);
        xSemaphoreGive(globalAppMutex);

        if (item) {
            streamer.setBaseFileName(baseName);
            streamer.setSingleFileDuration(singleFileDuration);
            streamer.startRecording(timestamp);

            Serial.print("Start recording: ");
            Serial.println(dateTime);
            Serial.print("Duration: ");
            Serial.println(item.getValue().duration);
        } else if (recorded) {
            streamer.tick(timestamp);
        } else if (streamer.stopRecording(timestamp)) {
            Serial.print("Stop recording: ");
            Serial.println(dateTime);
        }
    }
} // namespace

QueueHandle_t globalAppMutex;
DS3231& globalRtc                        = rtc;
TrackedValue<AppConfig>& globalAppConfig = appConfig;

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        // Waits for serial port to connect. Needed for native USB port only。
    }

    rtc.begin();
    loadAppConfig();
    initMultimedia();

    if (rtc.alarm1Triggered()) {
        rtc.clearAlarm1Flag();
    }

    WiFiHotspot.start(appConfig.value.hotspot.ssid, appConfig.value.hotspot.password, 1);

    webServer.setFallbackService(&fallbackService);
    webServer.start();

    liveStreamingServer.addService("GET /live", &videoStreamingService);
    liveStreamingServer.start();

    bleServer.addService(RequestType::getSystemInfo, &systemInfoService);
    bleServer.addService(RequestType::getRecordingSchedule, &currentScheduleService);
    bleServer.addService(RequestType::setSystemTime, &updateTimeService);
    bleServer.addService(RequestType::setRecordingSchedule, &updateScheduleService);
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

    driveRecording(unixTimestamp, dateTimeText);

    if (++counter % 10 == 0) {
        WiFiHotspot.dump();
        Serial.println(dateTimeText);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
}
