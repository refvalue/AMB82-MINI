#pragma once

#if 1
#include <cstddef>
#include <cstdint>
#include <ctime>

#include <WString.h>
#endif

#if 1
#include <StreamIO.h>
#endif

#include <MP4Recording.h>
#include <Optional.h>
#include <VideoStream.h>

class MixingStreamer {
public:
    MixingStreamer();
    ~MixingStreamer();
    void init(MMFModule videoInput, MMFModule mixedOutput, VideoSetting& videoSetting);

    void reset();
    uint32_t singleFileDuration();
    void setSingleFileDuration(uint32_t value);
    String baseFileName();
    void setBaseFileName(const String& value);
    void startRecording(int64_t timestamp);
    bool stopRecording(int64_t timestamp);
    void tick(int64_t timestamp);

private:
    void increaseFileName(bool reset = false);

    size_t index_;
    String baseFileName_;
    struct tm startTime_;
    Optional<int64_t> lastTimestamp_;

    MP4Recording mp4_;
    StreamIO avMixStreamer_;
};
