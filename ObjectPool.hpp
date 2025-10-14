#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

template <typename T, size_t PoolSize>
class ObjectPool {
public:
    ObjectPool() : freeCount_{PoolSize}, mutex_{xSemaphoreCreateMutex()} {
        for (size_t i = 0; i < PoolSize; i++) {
            freeIndices_[i] = i;
        }
    }

    T* acquire() {
        xSemaphoreTake(mutex_, portMAX_DELAY);

        T* item{};

        if (freeCount_ > 0) {
            const auto index = freeIndices_[--freeCount_];

            item = &pool_[index];
        }

        xSemaphoreGive(mutex_);

        return item;
    }

    void release(T* item) {
        xSemaphoreTake(mutex_, portMAX_DELAY);

        const auto index = item - &pool_[0];

        freeIndices_[freeCount_++] = index;
        xSemaphoreGive(mutex_);
    }

private:
    size_t freeCount_;
    std::array<T, PoolSize> pool_;
    std::array<size_t, PoolSize> freeIndices_;
    SemaphoreHandle_t mutex_;
};
