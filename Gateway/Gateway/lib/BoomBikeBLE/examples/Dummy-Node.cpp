// Simple dummy node to test BLE Long Range communication
// The device will periodically advertise itself with some data over BLE Long Range (Coded PHY)
// A BoomBike Gateway (running separate code) can scan for these advertisements and forward the data to InfluxDB

#include "BoomBikeBLE.h"
#include <Arduino.h>

// BLE communication
BoomBikeBLE bikeBLE("BoomBike-Gateway");

void setup() {
  Serial.begin(9600);
  Serial.println("Starting Dummy Node...");

  bikeBLE.begin();
  bikeBLE.setPhy(NimBLEScan::Phy::SCAN_CODED);

  bikeBLE.addService("ABCD");
  bikeBLE.addCharacteristic("ABCD", "1234", "0", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, 10);
  
  bikeBLE.startAdvertising();
  // TODO: Upgrade advertising to only advertise periodically to save power
}

void loop() {
  // Update characteristic "1234" every second with dummy data (uptime in seconds)
  bikeBLE.setCharacteristicValue("ABCD", "1234", String(millis() / 1000).c_str());
  delay(1000);
}
