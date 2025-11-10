#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

class AmebaFatFS;
struct QueueDefinition;

extern const char mainHtml[];
extern const char stylesCss[];
extern const char systemInfoHtml[];
extern const char scheduleHtml[];
extern const char liveStreamingHtml[];
extern const char notFoundHtml[];
extern const char jMuxerScript[];
extern const char appConfigFileName[];

extern AmebaFatFS& SDFs;
extern QueueDefinition* globalAppMutex;
extern std::atomic_int32_t globalNowSince2020;
extern std::atomic_int32_t globalPendingTimestampSince2020;

inline static constexpr size_t flashMemoryMappedSize = 0x1000;
