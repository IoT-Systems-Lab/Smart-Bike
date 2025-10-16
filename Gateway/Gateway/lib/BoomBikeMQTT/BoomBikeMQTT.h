// Connect to WiFi and MQTT broker
// Publish messages to MQTT broker
// by Kjell

#ifndef BOOMBIKEMQTT_H
#define BOOMBIKEMQTT_H

#include <AsyncMqttClient.h>
#include <WiFi.h>
#include <Arduino.h>

class BoomBikeMQTT {
public:
    BoomBikeMQTT(const char* ssid, const char* password, const char* mqtt_server_addr);
    void begin();
    void publishMessage(const char* topic, const char* payload);
private:
    const char* _ssid;
    const char* _password;
    const char* _mqtt_server;
    AsyncMqttClient _mqttClient;
    WiFiClient _wifiClient;

    void connectToMqtt();
    void connectToWifi();
    void onWifiEvent(arduino_event_t *event);
    void onMqttDisconnect(AsyncMqttClientDisconnectReason reason);
};

#endif // BOOMBIKEMQTT_H