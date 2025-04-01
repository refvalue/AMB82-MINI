#include "AppConfig.hpp"
#include "HttpServer.hpp"
#include "MixingStreamer.hpp"
#include "RecordingStateMachine.hpp"
#include "Resources.hpp"
#include "TrackedValue.hpp"
#include "WiFiHotspot.hpp"

#include <AmebaFatFS.h>
#include <VideoStream.h>
#include <VideoStreamOverlay.h>
#include <rtc.h>
#include <stdint.h>
#include <time.h>

extern HttpService& systemInfoService;
extern HttpService& currentScheduleService;
extern HttpService& updateTimeService;
extern HttpService& updateScheduleService;
extern HttpService& fallbackService;
extern HttpService& videoStreamingService;

extern MMFModule& videoStreamingMMFModule;

namespace {
    constexpr int32_t videoChannel = 0;

    TrackedValue<AppConfig> appConfig{AppConfig::createDefault()};

    VideoSetting videoSetting{videoChannel};
    MixingStreamer streamer;
    RecordingStateMachine recordingStateMachine;

    HttpServer webServer{80};
    HttpServer liveStreamingServer{8080};

    void initRTC() {
        rtc.Init();
    }

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
TrackedValue<AppConfig>& globalAppConfig = appConfig;

void setup() {
    Serial.begin(115200);

    while (!Serial) {
        // Waits for serial port to connect. Needed for native USB port only。
    }

    initRTC();
    loadAppConfig();
    initMultimedia();

    WiFiHotspot.start(appConfig.value.hotspot.ssid, appConfig.value.hotspot.password, 1);

    webServer.addService("GET /api/v1/systemInfo", &systemInfoService);
    webServer.addService("GET /api/v1/currentSchedule", &currentScheduleService);
    webServer.addService("POST /api/v1/updateTime", &updateTimeService);
    webServer.addService("POST /api/v1/updateSchedule", &updateScheduleService);
    webServer.setFallbackService(&fallbackService);
    webServer.start();

    liveStreamingServer.addService("GET /live", &videoStreamingService);
    liveStreamingServer.start();

    // Starts feeding multimedia data.
    Camera.channelBegin(videoChannel);
}

void loop() {
    static int32_t counter;
    static time_t timestamp;

    xSemaphoreTake(globalAppMutex, portMAX_DELAY);
    timestamp = rtc.Read();
    xSemaphoreGive(globalAppMutex);

    const auto dateTime = ctime(&timestamp);

    OSD.createBitmap(videoChannel);
    OSD.drawText(videoChannel, 36, 36, dateTime, 0xFFFFFFFF);
    OSD.update(videoChannel);

    driveRecording(timestamp, dateTime);

    if (++counter % 10 == 0) {
        WiFiHotspot.dump();
        Serial.println(dateTime);
    }

    vTaskDelay(500 / portTICK_PERIOD_MS);
}
