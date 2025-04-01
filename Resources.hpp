#pragma once

#include "TrackedValue.hpp"

class AmebaFatFS;
struct AppConfig;
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
extern TrackedValue<AppConfig>& globalAppConfig;
extern QueueDefinition* globalAppMutex;
