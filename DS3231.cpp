#include "DS3231.hpp"

#include <cstdint>

#include <Wire.h>

namespace {
    uint8_t bcdToDec(uint8_t val) noexcept {
        return ((val / 16 * 10) + (val % 16));
    }

    void decToBcd(uint8_t val, uint8_t& bcd) noexcept {
        bcd = ((val / 10) << 4) | (val % 10);
    }

    void setAlarm(TwoWire& wire, const DateTime& dateTime, uint8_t alarmReg) {
        wire.beginTransmission(0x68);
        wire.write(alarmReg);

        uint8_t bcd{};

        decToBcd(dateTime.second, bcd);
        wire.write(bcd & 0x7F);

        decToBcd(dateTime.minute, bcd);
        wire.write(bcd & 0x7F);

        decToBcd(dateTime.hour, bcd);
        wire.write(bcd & 0x7F);

        decToBcd(dateTime.day, bcd);
        wire.write(bcd & 0x3F);

        wire.endTransmission();
    }

    uint8_t readReg(TwoWire& wire, uint8_t reg) {
        wire.beginTransmission(0x68);
        wire.write(reg);
        wire.endTransmission();

        wire.requestFrom(0x68, 1);
        return wire.read();
    }

    void setReg(TwoWire& wire, uint8_t reg, uint8_t value) {
        wire.beginTransmission(0x68);
        wire.write(reg);
        wire.write(value);
        wire.endTransmission();
    }

    void enableAlarm(TwoWire& wire, bool enable, uint8_t controlMask) {
        auto control = readReg(wire, 0x0E);

        if (enable) {
            control |= controlMask;
        } else {
            control &= ~controlMask;
        }

        setReg(wire, 0x0E, control);
    }

    bool checkAlarmTriggered(TwoWire& wire, uint8_t flagMask) {
        const auto status = readReg(wire, 0x0F);
        return (status & flagMask) != 0;
    }

    void clearAlarmFlag(TwoWire& wire, uint8_t flagMask) {
        auto status = readReg(wire, 0x0F);
        status &= ~flagMask;
        setReg(wire, 0x0F, status);
    }
} // namespace

DS3231::DS3231(TwoWire& wire) : wire_{wire} {}

DS3231::~DS3231() = default;

void DS3231::begin() {
    wire_.begin();
}

DateTime DS3231::getDateTime() {
    wire_.beginTransmission(0x68);
    wire_.write(0x00);
    wire_.endTransmission();

    wire_.requestFrom(0x68, 7);

    const auto second    = bcdToDec(wire_.read() & 0x7F);
    const auto minute    = bcdToDec(wire_.read());
    const auto hour      = bcdToDec(wire_.read());
    const auto dayOfWeek = bcdToDec(wire_.read());
    const auto day       = bcdToDec(wire_.read());
    const auto month     = bcdToDec(wire_.read());
    const auto year      = static_cast<uint16_t>(bcdToDec(wire_.read()) + 2000);

    return {year, month, day, hour, minute, second};
}

void DS3231::setDateTime(const DateTime& dateTime) {
    wire_.beginTransmission(0x68);
    wire_.write(0x00);

    uint8_t bcd{};

    decToBcd(dateTime.second, bcd);
    wire_.write(bcd & 0x7F);

    decToBcd(dateTime.minute, bcd);
    wire_.write(bcd & 0x7F);

    decToBcd(dateTime.hour, bcd);
    wire_.write(bcd & 0x7F);

    decToBcd(1, bcd); // Day of week is not used.
    wire_.write(bcd & 0x07);

    decToBcd(dateTime.day, bcd);
    wire_.write(bcd & 0x3F);

    decToBcd(dateTime.month, bcd);
    wire_.write(bcd & 0x1F);

    decToBcd(static_cast<uint8_t>(dateTime.year - 2000), bcd);
    wire_.write(bcd);

    wire_.endTransmission();
}

void DS3231::enableAlarm1(bool enable) {
    enableAlarm(wire_, enable, 0x01);
}

void DS3231::enableAlarm2(bool enable) {
    enableAlarm(wire_, enable, 0x02);
}

void DS3231::setAlarm1(const DateTime& dateTime) {
    setAlarm(wire_, dateTime, 0x07);

    wire_.beginTransmission(0x68);
    wire_.write(0x0E);
    wire_.write(0x05); // Enables Alarm 1 interrupt.
    wire_.endTransmission();
}

void DS3231::setAlarm2(const DateTime& dateTime) {
    setAlarm(wire_, dateTime, 0x0B);

    wire_.beginTransmission(0x68);
    wire_.write(0x0E);
    wire_.write(0x06); // Enables Alarm 2 interrupt.
    wire_.endTransmission();
}

bool DS3231::alarm1Triggered() {
    return checkAlarmTriggered(wire_, 0x01);
}

bool DS3231::alarm2Triggered() {
    return checkAlarmTriggered(wire_, 0x02);
}

void DS3231::clearAlarm1Flag() {
    clearAlarmFlag(wire_, 0x01);
}

void DS3231::clearAlarm2Flag() {
    clearAlarmFlag(wire_, 0x02);
}
