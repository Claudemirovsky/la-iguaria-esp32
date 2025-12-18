#include "settings.h"
#include "EEPROM.h"
#include "esp_wifi_types.h"

void Settings::load() {
    uint8_t ssidLen = EEPROM.readByte(EEPROM_WIFI_SSID_LEN);
    if (ssidLen == 0 || ssidLen > MAX_SSID_LEN)
        return;
    wifiSSID = "";
    wifiPassword = "";
    userId = 0;
    for (uint8_t i = 0; i < ssidLen; ++i)
        wifiSSID += (char)EEPROM.readByte(EEPROM_WIFI_SSID + i);

    uint8_t passwordLen = EEPROM.readByte(EEPROM_WIFI_PASSWORD_LEN);
    if (passwordLen <= MAX_PASSPHRASE_LEN)
        for (uint8_t i = 0; i < passwordLen; ++i)
            wifiPassword += (char)EEPROM.readByte(EEPROM_WIFI_PASSWORD + i);
    Serial.println("EEPROM lido!");
    Serial.println("SSID: " + wifiSSID);
    Serial.println("Senha: " + wifiPassword);
}

void Settings::setWifiSSID(const String &ssid) {
    if (ssid.equals(this->wifiSSID))
        return;
    uint8_t len = ssid.length();
    EEPROM.writeByte(EEPROM_WIFI_SSID_LEN, len);
    this->wifiSSID = ssid;
    EEPROM.writeBytes(EEPROM_WIFI_SSID, ssid.c_str(), len);
}

void Settings::setWifiPassword(const String &password) {
    if (password.equals(this->wifiPassword))
        return;
    uint8_t len = password.length();
    EEPROM.writeByte(EEPROM_WIFI_PASSWORD_LEN, len);
    this->wifiPassword = password;
    EEPROM.writeBytes(EEPROM_WIFI_PASSWORD, password.c_str(), len);
}

String Settings::getWifiSSID() { return wifiSSID; }

String Settings::getWifiPassword() { return wifiPassword; }
