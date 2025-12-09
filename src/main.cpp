
#include "broker_mqtt.h"
#include "bt_manager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <MQTT.h>
#include <WiFi.h>
#include <time.h>

BTManager bt_manager;

String wifi_ssid = "";
String wifi_pass = "";
bool wifi_received = false;

WiFiClient wifiClient;
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
            String ssids[n];
            for (int i = 0; i < n; ++i)
                ssids[i] = WiFi.SSID(i);
            bt->notify(BTCOMMAND_WIFI_LIST, ssids, n);
        }; break;
        case BTCOMMAND_WIFI_AUTH:
            wifi_ssid = doc["ssid"].as<String>();
            if (!wifi_ssid) {
                bt->send_error(BTERROR_WIFI_INVALID_SSID);
                break;
            }
            wifi_pass = doc["password"].as<String>();
            wifi_received = true;

            Serial.println("SSID recebido: " + wifi_ssid);
            if (wifi_pass && !wifi_pass.isEmpty())
                Serial.println("Senha recebida: " + wifi_pass);
            bt->notify(BTCOMMAND_MESSAGE, "Credenciais OK");
            break;
        default:
            bt->send_error(BTERROR_INVALID_COMMAND);
            break;
        }
    }
}

void setupWiFi() {
    Serial.println("Conectando ao Wi-Fi...");
    Serial.println("SSID: " + wifi_ssid);
    Serial.println("SENHA: " + wifi_pass);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid, wifi_pass);

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
        configTime(0, 0, "pool.ntp.org");
    } else {
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
    uint32_t user_id = 666;
    JsonDocument doc;
    doc["user_id"] = user_id;
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

void setup(void) {
    Serial.begin(115200);

    bt_manager.setOnWriteCallBack(btmanager_callback);
    bt_manager.init("Esp32 Momento");
    Serial.printf("Advertising Started\n");
    mqtt.begin(mqtt_server, mqtt_port, wifiClient);
    mqtt.subscribe(mqtt_topic);
    mqtt.onMessage(mqtt_callback);
}

int counter = 0;
void loop() {
    delay(1000);
    if (wifi_received) {
        wifi_received = false;
        setupWiFi();
    }

    if (!mqtt.connected() && WiFi.status() == WL_CONNECTED) {
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
        send_pulse();
        delay(5000);
    }
}
