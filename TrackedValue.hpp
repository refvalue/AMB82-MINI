#pragma once

#include <atomic>
#include <memory>
#include <utility>

template <typename T>
class TrackedValue {
public:
    using ElementPair = std::pair<T, bool>;

    template <typename U>
    TrackedValue(U&& value) : value_{std::make_shared<ElementPair>(std::forward<U>(value), false)} {}

    std::shared_ptr<ElementPair> current() const {
        return std::atomic_load_explicit(&value_, std::memory_order::acquire);
    }

    template <typename U>
    void update(U&& value) {
        std::atomic_store_explicit(
            &value_, std::make_shared<ElementPair>(std::forward<U>(value), true), std::memory_order::release);
    }

    void confirm() {
        if (const auto current = std::atomic_load_explicit(&value_, std::memory_order::acquire); current->second) {
            std::atomic_store_explicit(
                &value_, std::make_shared<ElementPair>(current->first, false), std::memory_order::release);
        }
    }

private:
    std::shared_ptr<std::pair<T, bool>> value_;
};
