#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>

struct tskTaskControlBlock;

class ManagedTask {
public:
    static constexpr uint32_t defaultStackDepth = 4096;

    using CheckStoppedHandler = bool (*)();
    using TaskHandler         = std::function<void(CheckStoppedHandler checkStopped)>;

    ManagedTask();
    ManagedTask(TaskHandler handler, uint32_t stackDepth = defaultStackDepth);
    ManagedTask(const ManagedTask&) = delete;
    ManagedTask(ManagedTask&& other) noexcept;
    ~ManagedTask();
    ManagedTask& operator=(const ManagedTask&) = delete;
    ManagedTask& operator=(ManagedTask&& right) noexcept;
    void requestStop() const;
    void join();
    tskTaskControlBlock* handle() const noexcept;

private:
    static void taskRoutine(void* param);

    tskTaskControlBlock* task_;
};
