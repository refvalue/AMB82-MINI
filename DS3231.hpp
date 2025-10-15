#pragma once

#include "DateTime.hpp"

class TwoWire;

class DS3231 {
public:
    DS3231(TwoWire& wire);
    ~DS3231();
    void begin();
    DateTime getDateTime();
    void setDateTime(const DateTime& dateTime);
    void setAlarm1(const DateTime& dateTime);
    void setAlarm2(const DateTime& dateTime);
    bool alarm1Triggered();
    bool alarm2Triggered();
    void clearAlarm1Flag();
    void clearAlarm2Flag();

private:
    TwoWire& wire_;
};
