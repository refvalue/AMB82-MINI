#include "AppConfig.hpp"
#include "DateTime.hpp"
#include "HttpServer.hpp"
#include "LoRaHC15Service.hpp"
#include "LoRaService.hpp"
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
        Wire.begin();
    }

    uint8_t bcdToDec(uint8_t val) {
        return ((val / 16 * 10) + (val % 16));
    }

    DateTime getRtcTime() {
        Wire.beginTransmission(0x68);
        Wire.write(0x00);
        Wire.endTransmission();

        Wire.requestFrom(0x68, 7);

        const auto second    = bcdToDec(Wire.read() & 0x7F);
        const auto minute    = bcdToDec(Wire.read());
        const auto hour      = bcdToDec(Wire.read());
        const auto dayOfWeek = bcdToDec(Wire.read());
        const auto day       = bcdToDec(Wire.read());
        const auto month     = bcdToDec(Wire.read());
        const auto year      = static_cast<uint16_t>(bcdToDec(Wire.read()) + 2000);

        Serial.print("SECOND ");
        Serial.println((int) second);

        return {year, month, day, hour, minute, second};
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
    static time_t unixTimestamp;
    static DateTime dateTime;

    xSemaphoreTake(globalAppMutex, portMAX_DELAY);
    dateTime      = getRtcTime();
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
