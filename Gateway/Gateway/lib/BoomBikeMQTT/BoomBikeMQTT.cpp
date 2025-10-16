#include "BoomBikeMQTT.h"

BoomBikeMQTT::BoomBikeMQTT(const char* ssid, const char* password, const char* mqtt_server_addr)
    : _ssid(ssid), _password(password), _mqtt_server(mqtt_server_addr) {
    _mqttClient.onDisconnect([this](AsyncMqttClientDisconnectReason reason) { this->onMqttDisconnect(reason); });
}

void BoomBikeMQTT::begin() {
    Serial.begin(115200);
    Serial.println("Starting BoomBike Gateway...");

    // Set up WiFi
    WiFi.setHostname("boom-bike-gateway");
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connectToWifi();

    // MQTT configuration
    _mqttClient.setServer(_mqtt_server, 1883);
}

void BoomBikeMQTT::publishMessage(const char* topic, const char* payload) {
    // check if connected to MQTT
    int retryCount = 0;
    while (not _mqttClient.connected() && retryCount < 3) {
        Serial.println("MQTT not connected, attempting to reconnect...");
        connectToMqtt();
        delay(1000); // wait a bit before retrying
        retryCount++;
    }
    if (not _mqttClient.connected()) {
        Serial.println("Failed to connect to MQTT after 3 attempts.");
        return;
    }
    _mqttClient.publish(topic, 0, false, payload);
    Serial.printf("Published to topic %s: %s\n", topic, payload);
    
}

void BoomBikeMQTT::connectToMqtt() {
    // Ensure WiFi is connected before attempting MQTT connect
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected - will attempt to connect to WiFi first");
        connectToWifi();
        return; // wait for WiFi to come up; onWifiEvent will call connectToMqtt()
    }

    Serial.println("Connecting to MQTT broker...");
    _mqttClient.setClientId("boom-bike-gateway");
    _mqttClient.connect();
}

void BoomBikeMQTT::connectToWifi() {
    Serial.println("Connecting to WiFi...");
    WiFi.begin(_ssid, _password);
}


void BoomBikeMQTT::onMqttDisconnect(AsyncMqttClientDisconnectReason reason) {
    Serial.println("Disconnected from MQTT broker, reason: " + String(static_cast<int>(reason)));
    // Attempt to reconnect
    connectToMqtt();
}
