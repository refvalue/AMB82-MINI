#include "RecordingController.hpp"

#include "MixingStreamer.hpp"
#include "RecordingStateMachine.hpp"
#include "TimeUtil.hpp"

#include <cstdint>

#include <LOGUARTClass.h>
#include <PowerMode.h>

#if 1
#include <FreeRTOS.h>
#endif

#include <portmacro.h>

namespace {
    constexpr int32_t skippingThresholdSec    = 60;
    constexpr uint32_t delayMsBeforeDeepSleep = 15 * 60 * 1000;
} // namespace

class RecordingController::impl {
public:
    impl(DS3231& rtc, MixingStreamer& streamer) : rtc_{rtc}, initTime_{millis()}, streamer_{streamer} {}

    void update(const AppConfig::RecordingConfig& config) {
        stateMachine_.update(config.schedule);
        streamer_.setBaseFileName(config.baseName);
        streamer_.setSingleFileDuration(config.singleFileDuration);
    }

    void tick() {
        bool recorded{};
        const auto now = TimeUtil::toUnixTimestamp(rtc_.getDateTime());

        if (const auto item = stateMachine_.tryMatch(now, recorded)) {
            streamer_.startRecording(now);

            Serial.print("Start recording: ");
            Serial.println(TimeUtil::toIso8601(rtc_.getDateTime()));
            Serial.print("Duration: ");
            Serial.println(item->duration);

            return;
        }

        if (recorded) {
            return streamer_.tick(now);
        }

        if (streamer_.stopRecording(now)) {
            Serial.print("Stop recording: ");
            Serial.println(TimeUtil::toIso8601(rtc_.getDateTime()));
        }

        if (millis() - initTime_ > delayMsBeforeDeepSleep) {
            scheduleNextWakeup(now);
        }
    }

private:
    void scheduleNextWakeup(int64_t timestamp) {
        const auto nextPlan = stateMachine_.nextPending(timestamp);

        if (!nextPlan) {
            Serial.println("No upcoming recording plan. Sleeping indefinitely.");
            enterDeepSleep();
        }

        if (const auto secondsToNext = nextPlan->startTimestamp - timestamp; secondsToNext <= skippingThresholdSec) {
            Serial.print("Next recording in ");
            Serial.print(secondsToNext);
            Serial.println(" sec, skipping sleep.");
            return;
        }

        const DateTime wakeTime{nextPlan->startTimestamp - skippingThresholdSec};

        rtc_.clearAlarm1Flag();
        rtc_.setAlarm1(wakeTime);
        rtc_.enableAlarm1(true);

        Serial.print("Next wake scheduled at: ");
        Serial.println(TimeUtil::toIso8601(wakeTime));

        enterDeepSleep();
    }

    [[noreturn]] void enterDeepSleep() {
        Serial.println("Entering deep sleep...");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

        PowerMode.begin(DEEPSLEEP_MODE, 1, 0, 21);
        PowerMode.start();

        for (;;) {
        }
    }

    DS3231& rtc_;
    uint32_t initTime_;
    MixingStreamer& streamer_;
    RecordingStateMachine stateMachine_;
};

RecordingController::RecordingController(DS3231& rtc, MixingStreamer& streamer)
    : impl_(std::make_unique<impl>(rtc, streamer)) {}

RecordingController::RecordingController(RecordingController&&) noexcept            = default;
RecordingController::~RecordingController()                                         = default;
RecordingController& RecordingController::operator=(RecordingController&&) noexcept = default;

void RecordingController::update(const AppConfig::RecordingConfig& config) const {
    impl_->update(config);
}

void RecordingController::tick() const {
    impl_->tick();
}
