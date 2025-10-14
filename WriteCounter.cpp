#include "WriteCounter.hpp"

#include <AmebaFatFs.h>
#include <LOGUARTClass.h>

WriteCounter::WriteCounter(AmebaFatFS& fs, const String& filePath) : filePath_{filePath}, fs_{fs} {}

WriteCounter::~WriteCounter() = default;

void WriteCounter::increment() {
    ++counter_;

    if (auto file = fs_.open(filePath_)) {
        file.print("Written count: ");
        file.println(counter_);
    } else {
        Serial.print("Error opening `");
        Serial.print(filePath_);
        Serial.println("`.");
    }
}

uint64_t WriteCounter::count() const noexcept {
    return counter_;
}
