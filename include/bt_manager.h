#ifndef BT_MANAGER_H
#define BT_MANAGER_H
#include "ArduinoJson.h"
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <functional>

#define BTMANAGER_CALLBACK_SIGNATURE std::function<void(String, BTManager *)>
enum BTCommand {
    BTCOMMAND_ERROR = 1,
    BTCOMMAND_SUCCESS,
    BTCOMMAND_MESSAGE,
    BTCOMMAND_WIFI_LIST,
    BTCOMMAND_WIFI_AUTH,
    BTCOMMAND_USER_AUTH,
};

enum BTError {
    BTERROR_INVALID_DATA = 1,
    BTERROR_INVALID_COMMAND,
    BTERROR_WIFI_INVALID_SSID,
    BTERROR_WIFI_CONNECTION,
    BTERROR_WIFI_LIST,
    BTERROR_INVALID_USERID,
};

class BTManager {
  private:
    BTMANAGER_CALLBACK_SIGNATURE callback;

  public:
    BLECharacteristic *message_characteristic;
    BLECharacteristic *box_characteristic;
    BLEServer *ble_server;
    bool ble_connected;

    BTManager();

    void init(const char *);
    void setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE);
    inline void notify(enum BTCommand, std::function<void(JsonDocument &)>);
    void send_error(enum BTError);
    void notify(enum BTCommand, const char *);
    void notify(enum BTCommand, const String[], int len);
};
#endif
