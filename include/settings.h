#ifndef SETTINGS_H
#define SETTINGS_H
#include "esp_wifi_types.h"
#include <Arduino.h>
#include <EEPROM.h>
#include <stdint.h>

#define EEPROM_ADDR (0)
#define EEPROM_WIFI_SSID_LEN (EEPROM_ADDR)
#define EEPROM_WIFI_SSID (EEPROM_WIFI_SSID_LEN + 1)
#define EEPROM_WIFI_PASSWORD_LEN (EEPROM_WIFI_SSID + MAX_SSID_LEN + 1)
#define EEPROM_WIFI_PASSWORD (EEPROM_WIFI_PASSWORD_LEN + 1)
#define EEPROM_USER_ID (EEPROM_WIFI_PASSWORD + MAX_PASSPHRASE_LEN + 1)

class Settings {
  private:
    uint64_t userId = 1;
    String wifiSSID = "";
    String wifiPassword = "";

  public:
    void load();
    void setWifiSSID(const String &);
    void setWifiPassword(const String &);
    void setUserId(const uint64_t);
    void reset();
    String getWifiSSID();
    String getWifiPassword();
    uint64_t getUserId();
};
#endif
