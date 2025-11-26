
#include "broker_mqtt.h"
#include "bt_manager.h"
#include <Arduino.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>

BTManager bt_manager;

String wifi_ssid = "";
String wifi_pass = "";
bool wifi_received = false;

WiFiClientSecure wifiClient;
PubSubClient mqtt(wifiClient);

void btmanager_callback(String raw, BTManager *bt) {
    if (raw.length() > 0) {
        Serial.print("Recebido do app: ");
        Serial.println(raw);

        JsonDocument doc;
        DeserializationError err = deserializeJson(doc, raw);

        if (!err) {
            wifi_ssid = doc["ssid"].as<String>();
            wifi_pass = doc["password"].as<String>();
            wifi_received = true;

            Serial.println("SSID recebido: " + wifi_ssid);
            Serial.println("Senha recebida: " + wifi_pass);

            if (bt->ble_connected) {
                bt->notify("Credenciais OK");
            }
        } else {
            Serial.println("JSON inválido!");

            if (bt->ble_connected) {
                bt->notify("ERRO: JSON inválido");
            }
        }
    }
}

void setupWiFi() {
    Serial.println("Conectando ao Wi-Fi...");
    Serial.println("SSID: " + wifi_ssid);
    Serial.println("SENHA: " + wifi_pass);

    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());

    int timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 20) {
        delay(500);
        Serial.print(".");
        timeout++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi conectado!");
        Serial.println("IP: " + WiFi.localIP().toString());

        if (bt_manager.ble_connected) {
            bt_manager.notify(
                ("WiFi OK | IP: " + WiFi.localIP().toString()).c_str());
        }
    } else {
        Serial.println("\nFalha ao conectar ao Wi-Fi!");

        if (bt_manager.ble_connected) {
            bt_manager.notify("WiFI ERROR");
        }
    }
}

bool mqtt_connect() {
    String client_id = "esp32-client-";
    client_id += String(WiFi.macAddress());
    return mqtt.connect(client_id.c_str(), mqtt_user, mqtt_password);
}

void mqtt_callback(char *topic, byte *payload, uint32_t length) {
    Serial.printf("[%s]: ", topic);
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    Serial.println();
    Serial.println("-----------------------");
}

void setup(void) {
    Serial.begin(115200);

    bt_manager.setOnWriteCallBack(btmanager_callback);
    bt_manager.init("Esp32 Momento");
    Serial.printf("Advertising Started\n");
    wifiClient.setCACert(mqtt_cert);
    mqtt.setServer(mqtt_server, mqtt_port);
    mqtt.subscribe(mqtt_topic);
    mqtt.setCallback(mqtt_callback);
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
            Serial.print("Erro ao conectar ao MQTT: ");
            Serial.println(mqtt.state());
        }
    }

    if (mqtt.connected()) {
        mqtt.loop();
        char str[32];
        snprintf(str, 31, "Pulso: %d", ++counter);
        Serial.printf("Enviando mensagem: %s\n", str);
        mqtt.publish(mqtt_topic, str);
        delay(5000);
    }
}
