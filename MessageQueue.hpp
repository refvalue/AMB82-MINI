#pragma once

#include "ManagedTask.hpp"
#include "ObjectPool.hpp"

#include <cstddef>
#include <functional>

struct QueueDefinition;

class MessageQueue {
public:
    using Handler = std::function<void()>;

    MessageQueue(size_t capacity);
    ~MessageQueue();
    void beginInvoke(Handler handler);
    void beginInvokeFromIsr(Handler handler);

private:
    void taskRoutine(ManagedTask::CheckStoppedHandler checkStopped);

    ObjectPool<Handler> pool_;
    QueueDefinition* queue_;
    ManagedTask task_;
};
