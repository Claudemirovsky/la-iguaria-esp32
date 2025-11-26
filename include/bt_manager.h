#ifndef BT_MANAGER_H
#define BT_MANAGER_H
#include <NimBLEDevice.h>
#include <functional>

#define BTMANAGER_CALLBACK_SIGNATURE std::function<void(String, BTManager *)>
class BTManager {
  private:
    BTMANAGER_CALLBACK_SIGNATURE callback;

  public:
    NimBLECharacteristic *ble_input, *ble_output;
    NimBLEServer *ble_server;
    bool ble_connected;

    BTManager();

    void init(const char *);
    void setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE);
    void notify(const char *);
};
#endif
