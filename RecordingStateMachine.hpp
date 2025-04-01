#pragma once

#include "AppConfig.hpp"
#include "SafeList.hpp"

#include <DictionaryDeclarations.h>
#include <Optional.h>
#include <stdint.h>

class RecordingStateMachine {
public:
    void update(SafeList<AppConfig::RecordingPlan>& schedule);
    Optional<AppConfig::RecordingPlan> tryMatch(int64_t timestamp, bool& recorded);

private:
    Dictionary itemMapping_;
    Dictionary stateMapping_;
};
