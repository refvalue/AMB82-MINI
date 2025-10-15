#include "ManagedTask.hpp"

#if 1
#include <FreeRTOS.h>
#endif

#include <utility>

#include <WString.h>
#include <portmacro.h>
#include <task.h>

namespace {
    struct notificationBits {
        static constexpr uint32_t stopPending = 0x01;
        static constexpr uint32_t stopped     = 0x02;
    };
} // namespace

ManagedTask::ManagedTask() : param_{}, handler_{}, task_{}, creatingTask_{xTaskGetCurrentTaskHandle()} {}

ManagedTask::ManagedTask(TaskHandler handler, uint32_t stackDepth) : ManagedTask{handler, nullptr, stackDepth} {}

ManagedTask::ManagedTask(TaskHandler handler, void* param, uint32_t stackDepth)
    : param_{param}, handler_{handler}, creatingTask_{xTaskGetCurrentTaskHandle()} {
    xTaskCreate(&ManagedTask::taskRoutine, "Managed Task", stackDepth, this, 1, &task_);
}

ManagedTask::ManagedTask(ManagedTask&& other) noexcept
    : param_{std::exchange(other.param_, {})}, handler_{std::exchange(other.handler_, {})},
      task_{std::exchange(other.task_, {})}, creatingTask_{std::exchange(other.creatingTask_, {})} {}

ManagedTask::~ManagedTask() {
    requestStop();
    join();
}

ManagedTask& ManagedTask::operator=(ManagedTask&& right) noexcept {
    if (this != &right) {
        requestStop();
        join();
        param_        = std::exchange(right.param_, {});
        handler_      = std::exchange(right.handler_, {});
        task_         = std::exchange(right.task_, {});
        creatingTask_ = std::exchange(right.creatingTask_, {});
    }

    return *this;
}

void ManagedTask::requestStop() const {
    if (task_) {
        xTaskNotify(task_, notificationBits::stopPending, eSetBits);
    }
}

void ManagedTask::join() {
    if (task_) {
        uint32_t notifiedValue{};

        while (xTaskNotifyWait(0x00, 0xFFFFFFFF, &notifiedValue, portMAX_DELAY) != pdTRUE
               || (notifiedValue & notificationBits::stopped) == 0) {
        }

        task_ = nullptr;
    }
}

TaskHandle_t ManagedTask::handle() const noexcept {
    return task_;
}

void ManagedTask::taskRoutine(void* param) {
    auto&& self = *static_cast<ManagedTask*>(param);

    const auto checkStopped = [] {
        uint32_t notifiedValue{};

        return xTaskNotifyWait(0x00, 0xFFFFFFFF, &notifiedValue, 0) == pdTRUE
            && (notifiedValue & notificationBits::stopPending) == notificationBits::stopPending;
    };

    if (self.handler_) {
        self.handler_(checkStopped, self.param_);
    }

    xTaskNotify(self.creatingTask_, notificationBits::stopped, eSetBits);
    vTaskDelete(nullptr);
}
