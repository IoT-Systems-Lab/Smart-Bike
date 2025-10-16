// BoomBike Gateway
// Receive data from the bike over BLE
// Send data to the server over WiFi using MQTT

#include "BoomBikeMQTT.h"
#include "BoomBikeJSON.h"
#include "BoomBikeUltrasonic.h"
#include "BoomBikeBLE.h"
#include <Arduino.h>


// Network credentials
const char* ssid = "BoomBike";
const char* password = "KjellKjell";

// MQTT broker address
const char* mqtt_server = "192.168.137.1";
BoomBikeMQTT bikeMQTT(ssid, password, mqtt_server);

// BLE communication
BoomBikeBLE bikeBLE("BoomBike-Gateway");

// Data handling
BoomBikeJSON bikeJSON;

// Ultrasonic sensor on pins 8 (trigger) and 9 (echo)
BoomBikeUltrasonic ultrasonic(8, 9);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike Gateway...");

  bikeBLE.begin();
  bikeBLE.setPhy(NimBLEScan::Phy::SCAN_CODED);

  //bikeMQTT.begin();
  //ultrasonic.enablePassDetection();
  //ultrasonic.setTimeResolution(50); // read every 50 ms
}

void loop() {
  NimBLEAdvertisedDevice* device = bikeBLE.findDeviceWithService(NimBLEUUID("abcd"), 3); // Scan for 3 seconds to find the bike service
  if (device) {
    Serial.print("Found device: ");
    Serial.println(device->toString().c_str());
  } else {
    Serial.println("Device not found, retrying scan...");
    return; // Retry scanning
  }
  bikeBLE.connectToDevice(device);
  delay(4000); // Stay connected for 4 seconds
  bikeBLE.disconnect();
  delay(1000); // Wait 1 second before scanning again
  // bikeBLE.verboseScanForDevices(5000); // Scan for 5 seconds and print all found devices
  // delay(5000); // Wait 5 seconds before next scan
  

  // // No blocking work here — everything is event-driven
  // // send a message every 10 seconds
  // static unsigned long lastMsgTime = 0;
  // if (millis() - lastMsgTime > 10000) {
  //   Serial.println("Publishing MQTT message...");
  //   lastMsgTime = millis();
  //   bikeJSON.addData("speed", 24);
  //   bikeJSON.addData("temperature", 18.5);
  //   bikeJSON.addData("battery", 75);
  //   bikeMQTT.publishMessage("bike/data", bikeJSON.toString());
  //   bikeJSON.clear();
  // }

  // // Check for a pass event from the ultrasonic sensor
  // if (ultrasonic.checkForPass()) {
  //   Serial.println("Pass detected!");
  //   bikeJSON.addData("event", "pass");
  //   bikeMQTT.publishMessage("bike/data", bikeJSON.toString()); // immediately publish instead of batching
  //   bikeJSON.clear();
  // }

  // // send distance data every second
  // static unsigned long lastDistanceTime = 0;
  // if (millis() - lastDistanceTime > 1000) {
  //   lastDistanceTime = millis();
  //   float distance = ultrasonic.readDistance();
  //   bikeJSON.addData("distance", distance);
  //   bikeMQTT.publishMessage("bike/data", bikeJSON.toString());
  //   bikeJSON.clear();
  // }
}
