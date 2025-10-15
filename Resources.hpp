#pragma once

class DS3231;
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
extern DS3231& globalRtc;
