#include "settings.h"

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
    userId = EEPROM.readULong64(EEPROM_USER_ID);
    Serial.println("EEPROM lido!");
    Serial.println("SSID: " + wifiSSID);
    Serial.println("Senha: " + wifiPassword);
    Serial.printf("UID: %llu\n", userId);
}

void Settings::setUserId(const uint64_t userId) {
    if (userId != this->userId)
        EEPROM.writeULong64(EEPROM_USER_ID, userId);
    this->userId = userId;
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

uint64_t Settings::getUserId() { return userId; }

String Settings::getWifiSSID() { return wifiSSID; }

String Settings::getWifiPassword() { return wifiPassword; }
