#pragma once

#include "TrackedValue.hpp"

class AmebaFatFS;
class RtcDS3234;
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
//extern RtcDS3234& globalRtc;

RtcDS3234& getDS3234();
