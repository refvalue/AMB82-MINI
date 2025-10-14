#pragma once

#include "BtpTransport.hpp"
#include "ManagedTask.hpp"

#include <cstddef>
#include <cstdint>
#include <memory>

struct QueueDefinition;

namespace Btp {
    class BtpTransportScheduler {
    public:
        explicit BtpTransportScheduler(BtpTransport& transport);
        ~BtpTransportScheduler();

        bool start(const char* deviceName);
        void stop();
        bool send(const uint8_t* data, size_t size);

    private:
        void rxTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped);
        void txTaskRoutine(ManagedTask::CheckStoppedHandler checkStopped);

        BtpTransport& transport_;
        ManagedTask rxTask_;
        ManagedTask txTask_;
        QueueDefinition* txQueue_;
    };
} // namespace Btp
