#include "MixingStreamer.hpp"

#include "DateTime.hpp"
#include "Resources.hpp"
#include "TimeUtil.hpp"

#include <cstddef>
#include <cstdio>
#include <optional>

#if 1
#undef dbg_printf
#undef dbg_sprintf
#undef dbg_snprintf
#include <Arduino.h>
#endif

#include <AmebaFatFS.h>
#include <MP4Recording.h>
#include <StreamIO.h>
#include <VideoStream.h>

namespace {
    constexpr uint32_t defaultSingleFileDuration = 60 * 30;
    constexpr char defaultBaseFileName[]         = "recording";
} // namespace

class MixingStreamer::impl {
public:
    impl() : index_{}, baseFileName_{defaultBaseFileName}, startTime_{}, avMixStreamer_{1, 2} {}

    void init(MMFModule videoInput, MMFModule mixedOutput, VideoSetting& videoSetting) {
        reset();

        mp4_.configVideo(videoSetting);
        mp4_.setRecordingFileCount(1);
        mp4_.setRecordingDataType(STORAGE_VIDEO);

        avMixStreamer_.registerInput(videoInput);
        avMixStreamer_.registerOutput1(mixedOutput);
        avMixStreamer_.registerOutput2(mp4_);

        if (avMixStreamer_.begin() != 0) {
            Serial.println("Mixed StreamIO link start failed.");
        }
    }

    void reset() {
        avMixStreamer_.end();
        stopRecording(0);
    }

    uint32_t singleFileDuration() {
        return mp4_.getRecordingDuration();
    }

    void setSingleFileDuration(uint32_t value) {
        if (value == 0) {
            ++value;
        }
        mp4_.setRecordingDuration(value);
    }

    String baseFileName() {
        return baseFileName_;
    }

    void setBaseFileName(const String& value) {
        baseFileName_ = value;
    }

    void startRecording(int64_t timestamp) {
        stopRecording(timestamp);

        lastTimestamp_.reset();
        startTime_ = TimeUtil::toDateTime(timestamp);
        increaseFileName(true);
        mp4_.begin();
        tick(timestamp);
    }

    bool stopRecording(int64_t timestamp) {
        static DateTime dateTime;
        static char filePath[256];

        if (mp4_.getRecordingState()) {
            mp4_.end();

            while (mp4_.getRecordingState()) {
                vTaskDelay(1 / portTICK_RATE_MS);
            }

            std::snprintf(
                filePath, sizeof(filePath), "%s%s.mp4", SDFs.getRootPath(), mp4_.getRecordingFileName().c_str());

            if (SDFs.exists(filePath)) {
                dateTime = TimeUtil::toDateTime(timestamp);

                // Waiting for the file to be closed by the MP4 module.
                while (SDFs.setLastModTime(filePath, dateTime.year, dateTime.month, dateTime.day, dateTime.hour,
                           dateTime.minute, dateTime.second)
                       != 0) {
                    vTaskDelay(3 / portTICK_RATE_MS);
                }

                Serial.print("Set Last Modification Time: ");
                Serial.println(filePath);
            }

            return true;
        }

        return false;
    }

    void tick(int64_t timestamp) {
        if (!lastTimestamp_) {
            lastTimestamp_.emplace(timestamp);
        }

        if (timestamp - *lastTimestamp_ >= singleFileDuration()) {
            stopRecording(timestamp);
            increaseFileName();
            mp4_.begin();
            lastTimestamp_.emplace(timestamp);
        }
    }

private:
    void increaseFileName(bool reset = false) {
        static char fileName[128];

        if (reset) {
            index_ = {};
        }

        std::snprintf(fileName, sizeof(fileName), "%s_%04u-%02u-%02uT%02u-%02u-%02u_%u", baseFileName_.c_str(),
            startTime_.year, startTime_.month, startTime_.day, startTime_.hour, startTime_.minute, startTime_.second,
            index_++);
        mp4_.setRecordingFileName(fileName);
    }

    size_t index_;
    String baseFileName_;
    DateTime startTime_;
    std::optional<int64_t> lastTimestamp_;

    MP4Recording mp4_;
    StreamIO avMixStreamer_;
};

MixingStreamer::MixingStreamer() : impl_(std::make_unique<impl>()) {}

MixingStreamer::MixingStreamer(MixingStreamer&&) noexcept = default;

MixingStreamer::~MixingStreamer() = default;

MixingStreamer& MixingStreamer::operator=(MixingStreamer&&) noexcept = default;

void MixingStreamer::init(MMFModule videoInput, MMFModule mixedOutput, VideoSetting& videoSetting) const {
    impl_->init(videoInput, mixedOutput, videoSetting);
}

void MixingStreamer::reset() const {
    impl_->reset();
}

uint32_t MixingStreamer::singleFileDuration() const {
    return impl_->singleFileDuration();
}

void MixingStreamer::setSingleFileDuration(uint32_t value) const {
    impl_->setSingleFileDuration(value);
}

String MixingStreamer::baseFileName() const {
    return impl_->baseFileName();
}

void MixingStreamer::setBaseFileName(const String& value) const {
    impl_->setBaseFileName(value);
}

void MixingStreamer::startRecording(int64_t timestamp) const {
    impl_->startRecording(timestamp);
}

bool MixingStreamer::stopRecording(int64_t timestamp) const {
    return impl_->stopRecording(timestamp);
}

void MixingStreamer::tick(int64_t timestamp) const {
    impl_->tick(timestamp);
}
