#include "bt_manager.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define BT_INPUT_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BT_OUTPUT_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

class ServerCallbacks : public NimBLEServerCallbacks {
  public:
    BTManager *btmanager = nullptr;

    void setBTManager(BTManager *btm) { this->btmanager = btm; }
    void onConnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo) override {
        Serial.printf("Client address: %s\n",
                      connInfo.getAddress().toString().c_str());
        this->btmanager->ble_connected = true;
    }

    void onDisconnect(NimBLEServer *pServer, NimBLEConnInfo &connInfo,
                      int reason) override {
        Serial.printf("Client disconnected - start advertising\n");
        this->btmanager->ble_connected = false;
        NimBLEDevice::startAdvertising();
    }
} serverCallbacks;

class CharacteristicCallback : public NimBLECharacteristicCallbacks {
  public:
    BTMANAGER_CALLBACK_SIGNATURE onWriteCallBack;
    BTManager *btmanager = nullptr;

    void setBTManager(BTManager *btm) { this->btmanager = btm; }
    void setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE func) {
        this->onWriteCallBack = func;
    }

    void onWrite(NimBLECharacteristic *pCharacteristic,
                 NimBLEConnInfo &connInfo) override {
        String raw = pCharacteristic->getValue().c_str();
        Serial.printf("OnWrite called. onWrite is null? [%d] | btmanager is "
                      "null? [%d] | raw: %s\n",
                      this->onWriteCallBack == nullptr,
                      this->btmanager == nullptr, raw);
        if (this->onWriteCallBack != nullptr && this->btmanager != nullptr)
            this->onWriteCallBack(raw, this->btmanager);
    }
} chCallback;

BTManager::BTManager() : ble_connected(false) {}
void BTManager::setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE callback) {
    this->callback = callback;
}

void BTManager::init(const char *name) {
    serverCallbacks.setBTManager(this);
    chCallback.setBTManager(this);
    chCallback.setOnWriteCallBack(this->callback);
    NimBLEDevice::init(name);

    this->ble_server = NimBLEDevice::createServer();
    this->ble_server->setCallbacks(&serverCallbacks);

    NimBLEService *pService = this->ble_server->createService(SERVICE_UUID);
    this->ble_input = pService->createCharacteristic(
        BT_INPUT_UUID, NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR);

    this->ble_input->setCallbacks(&chCallback);

    this->ble_output = pService->createCharacteristic(
        BT_OUTPUT_UUID, NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::READ);

    pService->start();

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    advertising->setName(name);
    advertising->addServiceUUID(pService->getUUID());

    advertising->enableScanResponse(false);
    advertising->start();
}

void BTManager::notify(const char *message) {
    this->ble_output->setValue(message);
    this->ble_output->notify();
}
