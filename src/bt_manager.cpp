#include "bt_manager.h"

#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define MESSAGE_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define BOX_CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a9"

String boxValue = "0";

class MyServerCallbacks : public BLEServerCallbacks {
  public:
    BTManager *btmanager = nullptr;

    void setBTManager(BTManager *btm) { this->btmanager = btm; }

    void onConnect(BLEServer *pServer) override {
        Serial.println("Connected");
        if (this->btmanager != nullptr) {
            this->btmanager->ble_connected = true;
        }
    }

    void onDisconnect(BLEServer *pServer) override {
        Serial.println("Disconnected");
        if (this->btmanager != nullptr) {
            this->btmanager->ble_connected = false;
        }
        BLEDevice::startAdvertising();
    }
} serverCallbacks;

class CharacteristicsCallbacks : public BLECharacteristicCallbacks {
  public:
    BTMANAGER_CALLBACK_SIGNATURE onWriteCallBack;
    BTManager *btmanager = nullptr;

    void setBTManager(BTManager *btm) { this->btmanager = btm; }
    void setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE func) {
        this->onWriteCallBack = func;
    }

    void onWrite(BLECharacteristic *pCharacteristic) override {
        Serial.print("Value Written ");
        Serial.println(pCharacteristic->getValue().c_str());

        if (this->btmanager == nullptr)
            return;

        // Verifica qual characteristic foi escrito comparando diretamente
        if (pCharacteristic == this->btmanager->box_characteristic) {
            boxValue = pCharacteristic->getValue().c_str();
            pCharacteristic->setValue(const_cast<char *>(boxValue.c_str()));
            pCharacteristic->notify();
        } else if (pCharacteristic == this->btmanager->message_characteristic) {
            String raw = pCharacteristic->getValue().c_str();
            if (this->onWriteCallBack != nullptr)
                this->onWriteCallBack(raw, this->btmanager);
        }
    }
} chCallback;

BTManager::BTManager()
    : ble_connected(false), message_characteristic(nullptr),
      box_characteristic(nullptr), ble_server(nullptr) {}

void BTManager::setOnWriteCallBack(BTMANAGER_CALLBACK_SIGNATURE callback) {
    this->callback = callback;
}

void BTManager::init(const char *name) {
    serverCallbacks.setBTManager(this);
    chCallback.setBTManager(this);
    chCallback.setOnWriteCallBack(this->callback);

    // Create the BLE Device
    BLEDevice::init(name);

    // Create the BLE Server
    this->ble_server = BLEDevice::createServer();
    this->ble_server->setCallbacks(&serverCallbacks);

    // Create the BLE Service
    BLEService *pService = this->ble_server->createService(SERVICE_UUID);
    delay(100);

    // Create a BLE Characteristic
    this->message_characteristic = pService->createCharacteristic(
        MESSAGE_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY |
                                         BLECharacteristic::PROPERTY_INDICATE);

    this->box_characteristic = pService->createCharacteristic(
        BOX_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ |
                                     BLECharacteristic::PROPERTY_WRITE |
                                     BLECharacteristic::PROPERTY_NOTIFY |
                                     BLECharacteristic::PROPERTY_INDICATE);

    // Start the BLE service
    pService->start();

    // Start advertising
    this->ble_server->getAdvertising()->start();

    this->message_characteristic->setValue("Message one");
    this->message_characteristic->setCallbacks(&chCallback);

    this->box_characteristic->setValue("0");
    this->box_characteristic->setCallbacks(&chCallback);

    Serial.println("Waiting for a client connection to notify...");
}

inline void BTManager::notify(enum BTCommand cmd,
                              std::function<void(JsonDocument &)> function) {
    if (!this->ble_connected || this->message_characteristic == nullptr)
        return;
    JsonDocument doc;
    doc["cmd"] = cmd;
    function(doc);
    String out;
    serializeJson(doc, out);
    this->message_characteristic->setValue(out.c_str());
    this->message_characteristic->notify();
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
