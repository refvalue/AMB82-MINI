#include "ManagedTask.hpp"

#include <memory>
#include <utility>

#if 1
#include <FreeRTOS.h>
#endif

#include <WString.h>
#include <portmacro.h>
#include <task.h>

namespace {
    struct NotificationBits {
        static constexpr uint32_t stopPending = 0x01;
        static constexpr uint32_t stopped     = 0x02;
    };

    struct TaskPrivateData {
        ManagedTask::TaskHandler handler;
        TaskHandle_t creatingTask{};
    };
} // namespace

ManagedTask::ManagedTask() : task_{} {}

ManagedTask::ManagedTask(TaskHandler handler, uint32_t stackDepth) : ManagedTask{} {
    xTaskCreate(&ManagedTask::taskRoutine, "Managed Task", stackDepth,
        std::make_unique<TaskPrivateData>(std::move(handler), xTaskGetCurrentTaskHandle()).release(),
        tskIDLE_PRIORITY + 1, &task_);
}

ManagedTask::ManagedTask(ManagedTask&& other) noexcept : task_{std::exchange(other.task_, {})} {}

ManagedTask::~ManagedTask() {
    requestStop();
    join();
}

ManagedTask& ManagedTask::operator=(ManagedTask&& right) noexcept {
    if (this != &right) {
        requestStop();
        join();
        task_ = std::exchange(right.task_, {});
    }

    return *this;
}

void ManagedTask::requestStop() const {
    if (task_) {
        xTaskNotify(task_, NotificationBits::stopPending, eSetBits);
    }
}

void ManagedTask::join() {
    if (task_) {
        uint32_t notifiedValue{};

        while (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notifiedValue, portMAX_DELAY) != pdTRUE
               || (notifiedValue & NotificationBits::stopped) == 0) {
        }

        task_ = nullptr;
    }
}

TaskHandle_t ManagedTask::handle() const noexcept {
    return task_;
}

void ManagedTask::taskRoutine(void* param) {
    const auto checkStopped = [] {
        uint32_t notifiedValue{};

        return xTaskNotifyWait(0x00, 0xFFFFFFFF, &notifiedValue, 0) == pdTRUE
            && (notifiedValue & NotificationBits::stopPending) == NotificationBits::stopPending;
    };

    if (const std::unique_ptr<TaskPrivateData> data{static_cast<TaskPrivateData*>(param)}) {
        if (data->handler) {
            data->handler(checkStopped);
        }

        xTaskNotify(data->creatingTask, NotificationBits::stopped, eSetBits);
    }

    vTaskDelete(nullptr);
}
