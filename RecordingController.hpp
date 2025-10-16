#pragma once

#include "AppConfig.hpp"
#include "DS3231.hpp"

#include <memory>

class MixingStreamer;

class RecordingController {
public:
    RecordingController(DS3231& rtc, MixingStreamer& streamer);
    RecordingController(RecordingController&&) noexcept;
    ~RecordingController();
    RecordingController& operator=(RecordingController&&) noexcept;
    void update(const AppConfig::RecordingConfig& config) const;
    void tick() const;

private:
    class impl;

    std::unique_ptr<impl> impl_;
};
