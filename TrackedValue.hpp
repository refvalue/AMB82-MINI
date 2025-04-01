#pragma once

#include <utility>

template <typename T>
struct TrackedValue {
    bool updated;
    T value;

    template <typename U>
    TrackedValue(U&& value) : updated{}, value{std::forward<U>(value)} {}

    template <typename U>
    void update(U&& value) {
        this->value = std::forward<U>(value);
        updated     = true;
    }

    void markUpdated() {
        updated = true;
    }

    void confirm() {
        updated = false;
    }
};
