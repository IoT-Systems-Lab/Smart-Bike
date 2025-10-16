// Connect to the BoomBike via BLE
// Read data from the bike and make it available to other components
// note: to use this library: define build flag -D CONFIG_BT_NIMBLE_ENABLED
// note: for BLE Extended (Long range): define build flags -D CONFIG_BT_NIMBLE_EXT_ADV -D CONFIG_BT_NIMBLE_PHY_CODED
// by Kjell

#ifndef BOOMBIKEBLE_H
#define BOOMBIKEBLE_H

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <map>

// Forward declaration of ScanCallbacks
class ScanCallbacks;

class BoomBikeBLE {
private:
    String deviceName;
    std::map<String, NimBLEService*> services;
    NimBLEServer* pServer;
    NimBLEClient* pClient;
    ScanCallbacks* scanCallbacks;
    NimBLEScan::Phy scanPhy;

    friend class ScanCallbacks; // Allow ScanCallbacks to access private/protected members

public:
    BoomBikeBLE(String deviceName = "", NimBLEScan::Phy scanPhy = NimBLEScan::Phy::SCAN_ALL); // Empty name means no advertising (client mode), default scan all PHYs
    ~BoomBikeBLE();
    void begin();

    // Service and Characteristic management (Act as server)
    void addService(String serviceUUID);
    void addCharacteristic(String serviceUUID, String CharUUID, String value="", uint32_t properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, uint16_t max_len = BLE_ATT_ATTR_MAX_LEN);
    void setCharacteristicValue(String serviceUUID, String CharUUID, String value);
    void startAdvertising();

    // Client functionality
    NimBLEClient* getClient() { return pClient; }
    void setPhy(NimBLEScan::Phy phy) { scanPhy = phy; }
    void verboseScanForDevices(int durationSeconds = 10);
    NimBLEAdvertisedDevice* findDeviceWithService(const NimBLEUUID& serviceUUID, int scanDuration = 10);
    bool connectToDevice(NimBLEAdvertisedDevice* device);
    void disconnect();
};

// Class to handle the scan callbacks when advertisements are received
class ScanCallbacks : public NimBLEScanCallbacks {
    void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
        Serial.printf("Advertised Device found: %s\n PHY1: %s\n PHY2: %s\n",
                  advertisedDevice->toString().c_str(),
                  phyToString(advertisedDevice->getPrimaryPhy()),
                  phyToString(advertisedDevice->getSecondaryPhy()));
        Serial.println("--------------------------");
    }

    void onScanEnd(const NimBLEScanResults& scanResults, int reason) override {
        Serial.printf("Scan Ended, reason: %d; found %d devices\n", reason, scanResults.getCount());
        Serial.println("=====================================");
    }

private:
    String phyToString(uint8_t phy);
};

#endif // BOOMBIKEBLE_H
