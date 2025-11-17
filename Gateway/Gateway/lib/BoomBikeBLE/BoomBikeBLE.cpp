#include "BoomBikeBLE.h"
#include <Arduino.h>

BoomBikeBLE::BoomBikeBLE(const char* serviceUUID, const char* characteristicUUID)
    : m_serviceUUID(serviceUUID), m_characteristicUUID(characteristicUUID) {}

BoomBikeBLE::~BoomBikeBLE() {
    if (m_pAdvertisedDevice != nullptr) {
        delete m_pAdvertisedDevice;
        m_pAdvertisedDevice = nullptr;
    }
}

void BoomBikeBLE::begin() {
    NimBLEDevice::init("BoomBikeGateway");
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setScanCallbacks(this);
    pScan->setInterval(45);
    pScan->setWindow(15);
    pScan->setActiveScan(true);
    // Set the PHY to Coded for Long Range
    pScan->setPhy(NimBLEScan::Phy::SCAN_CODED);
}

void BoomBikeBLE::loop() {
    // If a device is found, connect to it from the main loop, not the callback
    if (m_doConnect) {
        m_doConnect = false;
        if (connectAndRead(m_pAdvertisedDevice)) {
            Serial.println("Successfully read data.");
        } else {
            Serial.println("Failed to read data.");
        }
        // Free the memory allocated for the device copy
        delete m_pAdvertisedDevice;
        m_pAdvertisedDevice = nullptr;

        // Restart the scan
        NimBLEDevice::getScan()->start(SCAN_DURATION, false, false);
        Serial.println("Scan restarted");
        return; // Return to avoid starting another scan immediately
    }

    uint32_t now = millis();
    // Check if it's time to start a new scan
    if (now - m_lastScanTime >= SCAN_INTERVAL) {
        m_lastScanTime = now;
        
        NimBLEScan* pScan = NimBLEDevice::getScan();
        // Start a non-blocking scan
        if (pScan->isScanning() == false) {
            pScan->start(SCAN_DURATION, false, false);
            Serial.println("Scan started");
        }
    }
}

void BoomBikeBLE::onDataReceived(DataReceivedCallback callback) {
    m_dataCallback = callback;
}

void BoomBikeBLE::setScanParameters(uint32_t intervalMs, uint32_t durationMs) {
    // Update scan parameters
    NimBLEScan* pScan = NimBLEDevice::getScan();
    pScan->setInterval(intervalMs);
    pScan->setWindow(durationMs);
}

void BoomBikeBLE::onResult(const NimBLEAdvertisedDevice* advertisedDevice) {
    
    // // Below is for debugging purposes
    // Serial.printf("Scan Result: %s RSSI: %d\n", advertisedDevice->getAddress().toString().c_str(), advertisedDevice->getRSSI());
    // // get service count
    // uint8_t serviceCount = advertisedDevice->getServiceUUIDCount();
    // Serial.printf("  Service UUID Count: %d\n", serviceCount);
    // // loop over services and print them
    // for (uint8_t i = 0; i < serviceCount; i++) {
    //     NimBLEUUID svcUUID = advertisedDevice->getServiceUUID(i);
    //     Serial.printf("    Service UUID[%d]: %s\n", i, svcUUID.toString().c_str());
    // }

    // Check if the device is advertising the service we're interested in
    if (advertisedDevice->isAdvertisingService(m_serviceUUID)) {
        Serial.printf("Found device: %s\n", advertisedDevice->getAddress().toString().c_str());
        // Stop scanning to connect
        NimBLEDevice::getScan()->stop();
        
        // Create a copy of the device object to use in the main loop
        m_pAdvertisedDevice = new NimBLEAdvertisedDevice(*advertisedDevice);
        // Set the flag to indicate that we want to connect
        m_doConnect = true;
    }
}

bool BoomBikeBLE::connectAndRead(const NimBLEAdvertisedDevice* device) {
    NimBLEClient* pClient = NimBLEDevice::createClient();
    if (!pClient) {
        Serial.println("Failed to create client");
        return false;
    }

    // Use Coded PHY for connection
    pClient->setConnectPhy(BLE_GAP_LE_PHY_CODED_MASK);
    pClient->setConnectTimeout(4 * 1000); // 4 seconds

    Serial.println("Connecting to device...");
    if (!pClient->connect(device)) {
        Serial.println("Failed to connect");
        NimBLEDevice::deleteClient(pClient);
        return false;
    }

    Serial.printf("Connected to %s\n", device->getAddress().toString().c_str());

    bool success = false;
    NimBLERemoteService* pSvc = pClient->getService(m_serviceUUID);
    if (pSvc) {
        NimBLERemoteCharacteristic* pChr = pSvc->getCharacteristic(m_characteristicUUID);
        if (pChr && pChr->canRead()) {
            std::string value = pChr->readValue();
            if (m_dataCallback) {
                // Pass the data to the main application via the callback
                m_dataCallback(device->getAddress().toString(), value);
            }
            success = true;
        }
    }

    pClient->disconnect();
    NimBLEDevice::deleteClient(pClient);
    Serial.println("Disconnected");
    return success;
}
