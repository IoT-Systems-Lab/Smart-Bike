// Scan for BLE devices and print the advertisement data

#include <Arduino.h>
#include "BoomBikeBLE.h"

NimBLEScan::Phy scanPhy = NimBLEScan::Phy::SCAN_CODED; // Choose between SCAN_1M, SCAN_CODED, SCAN_ALL

BoomBikeBLE bikeBLE(""); // No device name because we are a client (so no advertising needed)

void setup() {
    Serial.begin(115200);
    Serial.println("Starting BLE Scanner...");

    bikeBLE.begin();
    bikeBLE.setPhy(scanPhy);
}

void loop() {
    bikeBLE.verboseScanForDevices(2000); // scan for 2 seconds
    delay(8000); // wait 8 seconds before next scan
}
