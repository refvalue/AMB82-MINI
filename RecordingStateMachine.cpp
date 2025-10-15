#include "RecordingStateMachine.hpp"

#include "CryptoUtil.hpp"

#include <WString.h>

void RecordingStateMachine::update(std::span<AppConfig::RecordingPlan> schedule) {
    itemMapping_.destroy();

    for (auto&& item : schedule) {
        const auto key =
            CryptoUtil::sha256(String{static_cast<uint32_t>(static_cast<uint64_t>(item.startTimestamp) >> 32)}
                               + String{static_cast<uint32_t>(item.startTimestamp)} + String{item.duration});

        itemMapping_(key, reinterpret_cast<int32_t>(&item));
    }

    for (size_t i = 0; i < stateMapping_.size(); i++) {
        if (const auto key = stateMapping_.key(i); !itemMapping_(key)) {
            stateMapping_.remove(key.c_str());
        }
    }

    for (size_t i = 0; i < itemMapping_.size(); i++) {
        if (const auto key = itemMapping_.key(i); !stateMapping_(key)) {
            stateMapping_(key, static_cast<int32_t>(false));
        }
    }
}

std::optional<AppConfig::RecordingPlan> RecordingStateMachine::tryMatch(int64_t timestamp, bool& recorded) {
    std::optional<AppConfig::RecordingPlan> result;
    bool found{};

    recorded = false;

    for (size_t i = 0; i < itemMapping_.size(); i++) {
        const auto key       = itemMapping_.key(i);
        const auto recording = static_cast<bool>(stateMapping_[key].toInt());
        auto&& item          = *reinterpret_cast<AppConfig::RecordingPlan*>(itemMapping_.value(i).toInt());

        if (!found && timestamp >= item.startTimestamp && timestamp < item.startTimestamp + item.duration) {
            recorded = true;

            if (!recording) {
                stateMapping_(key, static_cast<int32_t>(true));
                result.emplace(item);
                found = true;
            }

            continue;
        }

        if (recording) {
            stateMapping_(key, static_cast<int32_t>(false));
        }
    }

    return result;
}
