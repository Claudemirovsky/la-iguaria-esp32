#ifndef SETTINGS_H
#define SETTINGS_H
#include "esp_wifi_types.h"
#include <Arduino.h>
#include <EEPROM.h>

#define EEPROM_ADDR (0)
#define EEPROM_WIFI_SSID_LEN (EEPROM_ADDR)
#define EEPROM_WIFI_SSID (EEPROM_WIFI_SSID_LEN + 1)
#define EEPROM_WIFI_PASSWORD_LEN (EEPROM_WIFI_SSID + MAX_SSID_LEN + 1)
#define EEPROM_WIFI_PASSWORD (EEPROM_WIFI_PASSWORD_LEN + 1)

class Settings {
  private:
    String wifiSSID = "";
    String wifiPassword = "";

  public:
    void load();
    void setWifiSSID(const String &);
    void setWifiPassword(const String &);
    void reset();
    String getWifiSSID();
    String getWifiPassword();
};
#endif
