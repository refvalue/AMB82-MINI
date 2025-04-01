/*

 Example guide:
 https://www.amebaiot.com/en/amebapro2-arduino-ap-mode/
 */

#include "WiFiHotspot.hpp"

#include <itoa.h>
#include <string.h>

namespace {
    constexpr uint8_t defaultIPAddress[] = {192, 168, 88, 1};
    constexpr uint8_t defaultSubnet[]    = {255, 255, 255, 0};
} // namespace

WiFiHotspotClass::WiFiHotspotClass(WiFiClass& wifi) : wifi_{wifi} {}

void WiFiHotspotClass::start(const String& ssid, const String& password, int32_t channel) const {
    start(ssid, password, channel, IPAddress{defaultIPAddress});
}

void WiFiHotspotClass::start(
    const String& ssid, const String& password, int32_t channel, const IPAddress& address) const {
    char inner_ssid[ssid.length() + 1]{};
    char inner_password[password.length() + 1]{};
    char inner_channel[16]{};

    strcpy(inner_ssid, ssid.c_str());
    strcpy(inner_password, password.c_str());
    itoa(channel, inner_channel, 10);

    wifi_.config(address, address, address, IPAddress{defaultSubnet});

    while (wifi_.apbegin(inner_ssid, inner_password, inner_channel) != WL_CONNECTED) {
        delay(10000);
    }
}

void WiFiHotspotClass::dump() const {
    // Dumps the SSID.
    Serial.print("SSID: ");
    Serial.println(wifi_.SSID());

    // Dumps the MAC address.
    byte bssid[6];

    wifi_.BSSID(bssid);
    Serial.print("BSSID: ");
    Serial.print(bssid[0], HEX);
    Serial.print(":");
    Serial.print(bssid[1], HEX);
    Serial.print(":");
    Serial.print(bssid[2], HEX);
    Serial.print(":");
    Serial.print(bssid[3], HEX);
    Serial.print(":");
    Serial.print(bssid[4], HEX);
    Serial.print(":");
    Serial.println(bssid[5], HEX);

    // Dumps the encryption type.
    const auto encryption = wifi_.encryptionType();

    Serial.print("Encryption Type:");
    Serial.println(encryption, HEX);
    Serial.println();

    // print your WiFi IP address:
    const auto ip = wifi_.localIP();

    Serial.print("IP Address: ");
    Serial.println(ip);

    // print your subnet mask:
    const auto subnet = wifi_.subnetMask();

    Serial.print("Net Mask: ");
    Serial.println(subnet);

    // print your gateway address:
    const auto gateway = wifi_.gatewayIP();

    Serial.print("Gateway: ");
    Serial.println(gateway);
    Serial.println();
}

WiFiHotspotClass WiFiHotspot{WiFi};
