#pragma once

#include <cstdint>
#include <memory>

#include <WString.h>

class MMFModule;
class VideoSetting;

class MixingStreamer {
public:
    MixingStreamer();
    MixingStreamer(MixingStreamer&&) noexcept;
    ~MixingStreamer();
    MixingStreamer& operator=(MixingStreamer&&) noexcept;
    void init(MMFModule videoInput, MMFModule mixedOutput, VideoSetting& videoSetting) const;
    void reset() const;
    uint32_t singleFileDuration() const;
    void setSingleFileDuration(uint32_t value) const;
    String baseFileName()const;
    void setBaseFileName(const String& value)const;
    void startRecording(int64_t timestamp)const;
    bool stopRecording(int64_t timestamp)const;
    void tick(int64_t timestamp)const;

private:
    class impl;

    std::unique_ptr<impl> impl_;
};
