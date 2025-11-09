#include "MessageQueue.hpp"

#include <utility>

#if 1
#include <FreeRTOS.h>
#endif

#include <queue.h>

MessageQueue::MessageQueue(size_t capacity)
    : pool_{capacity}, queue_{xQueueCreate(capacity, sizeof(Handler))},
      task_{std::bind_front(&MessageQueue::taskRoutine, this)} {}

MessageQueue::~MessageQueue() {
    task_.requestStop();
    task_ = {};

    if (queue_) {
        vQueueDelete(queue_);
        queue_ = nullptr;
    }
}

void MessageQueue::beginInvoke(Handler handler) {
    if (const auto item = pool_.acquire()) {
        *item = std::move(handler);
        xQueueSend(queue_, item, 0);
    }
}

void MessageQueue::beginInvokeFromIsr(Handler handler) {
    if (const auto item = pool_.acquire()) {
        *item = std::move(handler);
        xQueueSendFromISR(queue_, item, 0);
    }
}

void MessageQueue::taskRoutine(ManagedTask::CheckStoppedHandler checkStopped) {
    Handler* handler;

    while (!checkStopped()) {
        if (xQueueReceive(queue_, &handler, portMAX_DELAY) == pdTRUE && handler) {
            if (*handler) {
                (*handler)();
            }

            pool_.release(handler);
        }
    }
}
