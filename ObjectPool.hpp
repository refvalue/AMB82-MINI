#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#if 1
#include <FreeRTOS.h>
#endif

#include <semphr.h>

template <typename T>
class ObjectPool {
public:
    ObjectPool(size_t capacity)
        : freeCount_{capacity}, pool_(capacity), freeIndices_(capacity), mutex_{xSemaphoreCreateMutex()} {
        for (size_t i = 0; i < capacity; i++) {
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
    std::vector<T> pool_;
    std::vector<size_t> freeIndices_;
    SemaphoreHandle_t mutex_;
};
