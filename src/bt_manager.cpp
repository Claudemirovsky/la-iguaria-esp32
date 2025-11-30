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

inline void BTManager::notify(enum BTCommand cmd,
                              std::function<void(JsonDocument &)> function) {
    if (!this->ble_connected)
        return;
    JsonDocument doc;
    doc["cmd"] = cmd;
    function(doc);
    String out;
    serializeJson(doc, out);
    this->ble_output->setValue(out);
    this->ble_output->notify();
}

void BTManager::notify(enum BTCommand cmd, const char *message) {
    BTManager::notify(cmd, [&message](JsonDocument &doc) {
        if (message != nullptr)
            doc["data"] = message;
    });
}

void BTManager::send_error(enum BTError error) {
    BTManager::notify(BTCOMMAND_ERROR,
                      [error](JsonDocument &doc) { doc["data"] = error; });
}

void BTManager::notify(enum BTCommand cmd, const String data[], int len) {
    BTManager::notify(cmd, [&data, len](JsonDocument &doc) {
        JsonArray arr = doc["data"].to<JsonArray>();
        for (int i = 0; i < len; ++i)
            arr.add(data[i]);
    });
}
