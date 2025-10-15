#pragma once

#include <cstdint>

#include <IPAddress.h>
#include <WString.h>

// UTF-8 encoding can also be used for SSID with emoji characters
// Emoji characters can be converted into UTF-8 at https://mothereff.in/utf-8
// char ssid[] = "\xe2\x9c\x8c\xef\xb8\x8f Ameba \xe2\x9c\x8c\xef\xb8\x8f";

class WiFiClass;

class WiFiHotspotClass {
public:
    WiFiHotspotClass(WiFiClass& wifi);
    void start(const String& ssid, const String& password, int32_t channel) const;
    void start(const String& ssid, const String& password, int32_t channel, const IPAddress& address) const;
    void stop() const;
    void dump() const;

private:
    WiFiClass& wifi_;
};

extern WiFiHotspotClass WiFiHotspot;
