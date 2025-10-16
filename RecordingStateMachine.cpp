#include "RecordingStateMachine.hpp"

#include "CryptoUtil.hpp"

#include <limits>
#include <utility>

#undef min
#undef max

void RecordingStateMachine::update(std::span<const AppConfig::RecordingPlan> schedule) {
    itemMapping_.clear();

    for (auto&& item : schedule) {
        const auto key =
            CryptoUtil::sha256(String{static_cast<uint32_t>(static_cast<uint64_t>(item.startTimestamp) >> 32)}
                               + String{static_cast<uint32_t>(item.startTimestamp)} + String{item.duration});

        itemMapping_.insert_or_assign(key, item);
    }

    for (auto&& [key, value] : stateMapping_) {
        if (!itemMapping_.contains(key)) {
            stateMapping_.erase(key);
        }
    }

    for (auto&& [key, value] : itemMapping_) {
        if (!stateMapping_.contains(key)) {
            stateMapping_.insert_or_assign(key, false);
        }
    }
}

std::optional<AppConfig::RecordingPlan> RecordingStateMachine::tryMatch(int64_t timestamp, bool& recorded) {
    std::optional<AppConfig::RecordingPlan> result;
    bool found{};
    recorded = false;

    for (auto&& [key, value] : itemMapping_) {
        const auto recording = stateMapping_[key];

        if (!found && timestamp >= value.startTimestamp && timestamp < value.startTimestamp + value.duration) {
            recorded = true;

            if (!recording) {
                stateMapping_.insert_or_assign(key, true);
                result.emplace(value);
                found = true;
            }

            continue;
        }

        if (recording) {
            stateMapping_.insert_or_assign(key, false);
        }
    }

    return result;
}

std::optional<AppConfig::RecordingPlan> RecordingStateMachine::nextPending(int64_t timestamp) {
    auto nearestStart = std::numeric_limits<int64_t>::max();
    std::optional<std::reference_wrapper<AppConfig::RecordingPlan>> nearest;

    for (auto&& [key, value] : itemMapping_) {
        const auto recording = stateMapping_[key];

        if (!recording && value.startTimestamp > timestamp && value.startTimestamp < nearestStart) {
            nearestStart = value.startTimestamp;
            nearest.emplace(value);
        }
    }

    if (nearest) {
        return *nearest;
    }

    return std::nullopt;
}
