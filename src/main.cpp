
#include "broker_mqtt.h"
#include "bt_manager.h"
#include "settings.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <WiFiClientSecure.h>
#include <cstdint>
#include <time.h>

#define LDR_PIN 15
#define HOUR 3600
#define FORTALEZA_OFFSET_GMT -3
#define TIMEZONE_OFFSET (FORTALEZA_OFFSET_GMT * HOUR)
BTManager bt_manager;
Settings settings;

bool wifi_received = false;
bool time_sync = false;
WiFiClientSecure wifiClient;
MQTTClient mqtt(256);

void btmanager_callback(String raw, BTManager *bt) {
    if (raw.length() > 0) {
        Serial.print("Recebido do app: ");
        Serial.println(raw);

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, raw);

        if (err || !doc["cmd"]) {
            Serial.println("JSON inválido!");

            bt->send_error(BTERROR_INVALID_DATA);
            return;
        }

        switch (doc["cmd"].as<int>()) {
        case BTCOMMAND_WIFI_LIST: {
            int n = WiFi.scanNetworks();
            Serial.printf("WiFis detectados: %d\n", n);
            if (n == WIFI_SCAN_FAILED) {
                bt->send_error(BTERROR_WIFI_LIST);
                break;
            }
            String ssids[n];
            for (int i = 0; i < n; ++i) {
                ssids[i] = WiFi.SSID(i);
                Serial.printf("%02d) %s\n", i + 1, ssids[i].c_str());
            }
            bt->notify(BTCOMMAND_WIFI_LIST, ssids, n);
        }; break;

        case BTCOMMAND_WIFI_AUTH: {
            String ssid = doc["ssid"].as<String>();
            if (!ssid) {
                bt->send_error(BTERROR_WIFI_INVALID_SSID);
                break;
            }
            String password = doc["password"].as<String>();
            settings.setWifiSSID(ssid);

            Serial.println("SSID recebido: " + ssid);
            if (password && !password.isEmpty())
                Serial.println("Senha recebida: " + password);
            bt->notify(BTCOMMAND_MESSAGE, "Credenciais OK");
            settings.setWifiSSID(ssid);
            settings.setWifiPassword(password);
            wifi_received = true;
            EEPROM.commit();
        }; break;
        case BTCOMMAND_USER_AUTH: {
            uint64_t uid = doc["userId"].as<uint64_t>();
            settings.setUserId(uid);
            if (EEPROM.commit()) {
                String msg = "UserId recebido: ";
                msg += String(uid);
                bt->notify(BTCOMMAND_SUCCESS, msg.c_str());
                Serial.println(msg);

            } else {
                bt->send_error(BTERROR_INVALID_USERID);
            }
        }; break;

        default:
            bt->send_error(BTERROR_INVALID_COMMAND);
            break;
        }
    }
}

void setupWiFi() {
    Serial.println("Conectando ao Wi-Fi...");
    Serial.println("SSID: " + settings.getWifiSSID());
    Serial.println("SENHA: " + settings.getWifiPassword());

    WiFi.mode(WIFI_STA);
    WiFi.begin(settings.getWifiSSID(), settings.getWifiPassword());

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
        String ipAddress = WiFi.localIP().toString();
        Serial.println("IP: " + ipAddress);

        bt_manager.notify(BTCOMMAND_SUCCESS,
                          ("WiFi OK | IP: " + ipAddress).c_str());
        configTime(TIMEZONE_OFFSET, 0, "pool.ntp.org");
        Serial.println("Horário configurado!");
        time_sync = true;
    } else {
        WiFi.disconnect();
        Serial.println("\nFalha ao conectar ao Wi-Fi!");
        bt_manager.send_error(BTERROR_WIFI_CONNECTION);
    }
}

int64_t getEpochMillis() {
    struct timeval tv;
    gettimeofday(&tv, NULL);

    return (int64_t)(tv.tv_sec) * 1000 + tv.tv_usec / 1000;
}

void send_pulse() {
    JsonDocument doc;
    doc["userId"] = settings.getUserId();
    doc["timestamp"] = getEpochMillis();
    String out;
    serializeJson(doc, out);
    Serial.printf("Enviando mensagem: %s\n", out.c_str());
    mqtt.publish(mqtt_topic, out);
}

bool mqtt_connect() {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    return mqtt.connect(client_id.c_str(), mqtt_user, mqtt_password);
}

void mqtt_callback(String &topic, String &payload) {
    Serial.printf("[%s]: %s", topic.c_str(), payload.c_str());
    Serial.println("-----------------------");
}

uint8_t lastState = 0;
void setup(void) {
    Serial.begin(115200);
    pinMode(LDR_PIN, INPUT);
    bt_manager.setOnWriteCallBack(btmanager_callback);
    bt_manager.init("Esp32 Momento");
    Serial.printf("Advertising Started\n");
    mqtt.begin(mqtt_server, mqtt_port, wifiClient);
    mqtt.subscribe(mqtt_topic);
    mqtt.onMessage(mqtt_callback);
    wifiClient.setCACert(mqtt_cert);
    lastState = digitalRead(LDR_PIN);
    EEPROM.begin(256);
    settings.load();
    if (settings.getWifiSSID().length()) {
        wifi_received = true;
    }
}

bool detect_pulse() {
    uint8_t state = digitalRead(LDR_PIN);
    if (state != lastState) {
        delay(50); // Anti ruído
        state = digitalRead(LDR_PIN);
        if (state != lastState) {
            lastState = state;
            return state == LOW; // LOW = tem luz, HIGH = sem luz
        }
    }
    return false;
}
void loop() {
    if (wifi_received) {
        wifi_received = false;
        setupWiFi();
    }

    if (!mqtt.connected() && WiFi.status() == WL_CONNECTED && time_sync) {
        if (mqtt_connect()) {
            Serial.println("Conectado ao MQTT!");
        } else {
            Serial.printf("Erro ao conectar ao MQTT: %d\n", mqtt.lastError());
        }
    }

    if (mqtt.connected()) {
        if (!mqtt.loop()) {
            Serial.printf("Erro no loop do mqtt: %d", mqtt.lastError());
            return;
        }
        if (detect_pulse())
            send_pulse();
    }
}
