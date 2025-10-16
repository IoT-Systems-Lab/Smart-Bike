// Example BLE Server using BoomBikeBLE library
// Create a BLE server that advertises services and characteristics
// Service with UID "0002" and characteristic "C047" that updates every second and can be read and notified
// For BLE Extended Advertising and Coded PHY (long range): enable the following build flags in platformio.ini
// build_flags = -D CONFIG_BT_NIMBLE_ENABLED -D CONFIG_BT_NIMBLE_EXT_ADV -D CONFIG_BT_NIMBLE_PHY_CODED
// by Kjell

#include <Arduino.h>
#include "BoomBikeBLE.h"

BoomBikeBLE bikeBLE("BoomBike-Gateway");

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BoomBike BLE Server...");

    bikeBLE.begin();

    bikeBLE.addService("ABCD");
    bikeBLE.addCharacteristic("ABCD", "1234"); // default properties: read and write; default max_len: 512
    bikeBLE.setCharacteristicValue("ABCD", "1234", "Example Value");

    bikeBLE.addCharacteristic("0001", "0123", "Initial Value", NIMBLE_PROPERTY::READ, 128); // example of custom properties and max_len
    bikeBLE.addCharacteristic("0001", "AABB", "Hello from BoomBike Gateway", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, 128);
    bikeBLE.addCharacteristic("0002", "C047", "0", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, 10);

    bikeBLE.startAdvertising();
}

void loop() {
    // Update characteristic "C047" every second with the current time in seconds
    bikeBLE.setCharacteristicValue("0002", "C047", String(millis() / 1000).c_str());
    delay(1000);
}
