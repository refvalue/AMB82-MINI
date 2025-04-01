#include "MixingStreamer.hpp"
#include "Resources.hpp"

#include <AmebaFatFS.h>
#include <stdio.h>

namespace {
    constexpr uint32_t defaultSingleFileDuration = 60 * 30;
    constexpr char defaultBaseFileName[]         = "recording";
} // namespace

MixingStreamer::MixingStreamer() : index_{}, baseFileName_{defaultBaseFileName}, startTime_{}, avMixStreamer_{1, 2} {}

MixingStreamer::~MixingStreamer() {
    reset();
}

void MixingStreamer::init(MMFModule videoInput, MMFModule mixedOutput, VideoSetting& videoSetting) {
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

void MixingStreamer::reset() {
    avMixStreamer_.end();
    stopRecording(0);
}

uint32_t MixingStreamer::singleFileDuration() {
    return mp4_.getRecordingDuration();
}

void MixingStreamer::setSingleFileDuration(uint32_t value) {
    if (value == 0) {
        ++value;
    }

    mp4_.setRecordingDuration(value);
}

String MixingStreamer::baseFileName() {
    return baseFileName_;
}

void MixingStreamer::setBaseFileName(const String& value) {
    baseFileName_ = value;
}

void MixingStreamer::startRecording(int64_t timestamp) {
    stopRecording(timestamp);

    lastTimestamp_.clear();
    gmtime_r(&timestamp, &startTime_);
    increaseFileName(true);
    mp4_.begin();
    tick(timestamp);
}

bool MixingStreamer::stopRecording(int64_t timestamp) {
    static struct tm tm;
    static char filePath[256];

    if (mp4_.getRecordingState()) {
        mp4_.end();

        while (mp4_.getRecordingState()) {
            vTaskDelay(1 / portTICK_RATE_MS);
        }

        sprintf(filePath, "%s%s.mp4", SDFs.getRootPath(), mp4_.getRecordingFileName().c_str());

        if (SDFs.exists(filePath)) {
            gmtime_r(&timestamp, &tm);

            // Waiting for the file to be closed by the MP4 module.
            while (SDFs.setLastModTime(filePath, static_cast<uint16_t>(tm.tm_year + 1900),
                       static_cast<uint16_t>(tm.tm_mon), static_cast<uint16_t>(tm.tm_mday),
                       static_cast<uint16_t>(tm.tm_hour), static_cast<uint16_t>(tm.tm_min),
                       static_cast<uint16_t>(tm.tm_sec))
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

void MixingStreamer::tick(int64_t timestamp) {
    if (!lastTimestamp_) {
        lastTimestamp_.emplace(timestamp);
    }

    if (timestamp - lastTimestamp_.getValue() >= singleFileDuration()) {
        stopRecording(timestamp);
        increaseFileName();
        mp4_.begin();
        lastTimestamp_.emplace(timestamp);
    }
}

void MixingStreamer::increaseFileName(bool reset) {
    static char fileName[128];

    if (reset) {
        index_ = {};
    }

    sprintf(fileName, "%s_%04d-%02d-%02dT%02d-%02d-%02d_%u", baseFileName_.c_str(), startTime_.tm_year + 1900,
        startTime_.tm_mon, startTime_.tm_mday, startTime_.tm_hour, startTime_.tm_min, startTime_.tm_sec, index_++);
    mp4_.setRecordingFileName(fileName);
}
