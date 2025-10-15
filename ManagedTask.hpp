#pragma once

#include <cstddef>
#include <cstdint>

struct tskTaskControlBlock;

class ManagedTask {
public:
    static constexpr uint32_t defaultStackDepth = 4096;

    using CheckStoppedHandler  = bool (*)();
    using TaskHandler          = void (*)(CheckStoppedHandler checkStopped, void* param);

    ManagedTask();
    ManagedTask(TaskHandler handler, uint32_t stackDepth = defaultStackDepth);
    ManagedTask(TaskHandler handler, void* param, uint32_t stackDepth = defaultStackDepth);
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

    void* param_;
    TaskHandler handler_;
    tskTaskControlBlock* task_;
    tskTaskControlBlock* creatingTask_;
};
