#pragma once

#include "AppConfig.hpp"

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

#include <DictionaryDeclarations.h>

class RecordingStateMachine {
public:
    void update(std::span<AppConfig::RecordingPlan> schedule);
    std::optional<AppConfig::RecordingPlan> tryMatch(int64_t timestamp, bool& recorded);

private:
    Dictionary itemMapping_;
    Dictionary stateMapping_;
};
