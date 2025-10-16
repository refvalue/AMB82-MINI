#pragma once

#include "AppConfig.hpp"
#include "HashUtil.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>

#include <WString.h>

class RecordingStateMachine {
public:
    void update(std::span<const AppConfig::RecordingPlan> schedule);
    std::optional<AppConfig::RecordingPlan> tryMatch(int64_t timestamp, bool& recorded);
    std::optional<AppConfig::RecordingPlan> nextPending(int64_t timestamp);

private:
    std::unordered_map<String, AppConfig::RecordingPlan, HashUtil::StringHash> itemMapping_;
    std::unordered_map<String, bool, HashUtil::StringHash> stateMapping_;
};
