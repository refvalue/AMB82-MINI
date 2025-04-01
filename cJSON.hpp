#pragma once

#include <cJSON.h>

class cJSONPtr {
public:
    explicit cJSONPtr(cJSON* ptr, bool copy = false)
        : ptr_{copy ? (ptr ? cJSON_CreateObjectReference(ptr) : nullptr) : ptr} {}

    cJSONPtr(const cJSONPtr& other) : cJSONPtr{other.ptr_, true} {}

    cJSONPtr(cJSONPtr&& other) noexcept : ptr_{other.ptr_} {
        other.ptr_ = nullptr;
    }

    ~cJSONPtr() {
        reset();
    };

    cJSONPtr& operator=(const cJSONPtr& right) {
        reset();

        if (right.ptr_) {
            ptr_ = cJSON_CreateObjectReference(right.ptr_);
        }

        return *this;
    }

    cJSONPtr& operator=(cJSONPtr&& right) noexcept {
        const auto intermediate = ptr_;

        ptr_       = right.ptr_;
        right.ptr_ = intermediate;

        return *this;
    }

    explicit operator bool() const noexcept {
        return ptr_;
    }

    void reset() {
        if (ptr_) {
            cJSON_Delete(ptr_);
            ptr_ = nullptr;
        }
    }

    cJSON* get() const noexcept {
        return ptr_;
    }

    cJSON* detach() noexcept {
        auto ptr = ptr_;

        return (ptr_ = nullptr, ptr);
    }

private:
    cJSON* ptr_;
};
