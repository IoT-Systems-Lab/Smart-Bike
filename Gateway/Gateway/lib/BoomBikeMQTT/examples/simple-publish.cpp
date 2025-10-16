// Simple example to publish MQTT messages using BoomBikeMQTT library

#include "BoomBikeMQTT.h"

// Network credentials
const char* ssid = "WiFi_SSID";
const char* password = "WiFi_PASSWORD";

// MQTT broker address
const char* mqtt_server = "MQTT_BROKER_ADDRESS"; // e.g., "broker.hivemq.com" or "192.168.1.100"

BoomBikeMQTT bikeMQTT(ssid, password, mqtt_server);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike MQTT Example...");
  bikeMQTT.begin();
}

void loop() {
  // No blocking work here — everything is event-driven
  // send a test message every 10 seconds
  static unsigned long lastMsgTime = 0;
  if (millis() - lastMsgTime > 10000) {
    Serial.println("Publishing MQTT message...");
    lastMsgTime = millis();
    bikeMQTT.publishMessage("test/topic", "{\"message\": \"Hello from BoomBikeMQTT!\"}");
  }
}