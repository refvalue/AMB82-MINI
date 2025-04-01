#pragma once

#include <tuple>
#include <utility>

#include <FreeRTOS.h>

template <typename... Mutexes>
class ScopedLock {
public:
    ScopedLock(Mutexes&... mutexes) : mutexes_{mutexes...} {
        (xSemaphoreTake(mutexes, portMAX_DELAY), ...);
    }

    ~ScopedLock() {
        std::apply([](auto&... args) { (xSemaphoreGive(args), ...); }, std::move(mutexes_));
    }

private:
    std::tuple<Mutexes&...> mutexes_;
}

template <typename... Mutexes>
ScopedLock(Mutexes&... mutexes) -> ScopedLock<Mutexes...>;
