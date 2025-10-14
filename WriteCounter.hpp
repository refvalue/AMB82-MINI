#pragma once

#include <cstdint>

#include <WString.h>

class AmebaFatFS;

class WriteCounter {
public:
    WriteCounter(AmebaFatFS& fs, const String& filePath);
    ~WriteCounter();
    void increment();
    uint64_t count() const noexcept;

private:
    uint64_t counter_;
    String filePath_;
    AmebaFatFS& fs_;
};
