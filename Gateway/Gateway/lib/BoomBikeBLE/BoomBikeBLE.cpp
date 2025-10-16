#include "BoomBikeBLE.h"

BoomBikeBLE::BoomBikeBLE(String deviceName, NimBLEScan::Phy scanPhy) {
    this->deviceName = deviceName;
    this->scanPhy = scanPhy;
    scanCallbacks = new ScanCallbacks();
    services = std::map<String, NimBLEService*>();
    pServer = nullptr;
    pClient = nullptr;
}

BoomBikeBLE::~BoomBikeBLE() {
    if (pClient) {
        NimBLEDevice::deleteClient(pClient);
        pClient = nullptr;
    }
    if (pServer) {
        NimBLEDevice::deinit(true); // Deinitialize NimBLE and clear all data
        pServer = nullptr;
    }
    services.clear();
}

void BoomBikeBLE::begin() {
    NimBLEDevice::init(deviceName.length() ? deviceName.c_str() : "");
}

void BoomBikeBLE::addService(String serviceUUID) {
    // Create server if it doesn't exist
    if (pServer == nullptr) {
        pServer = NimBLEDevice::createServer();
    }
    // Check if service already exists
    NimBLEService* existingService = nullptr;
    if (pServer) existingService = pServer->getServiceByUUID(serviceUUID.c_str());
    if (existingService != nullptr) {
        Serial.println("Service already exists: " + serviceUUID);
        return;
    }
    // Create and store the service
    NimBLEService* pService = nullptr;
    if (pServer) pService = pServer->createService(serviceUUID.c_str());
    if (!pService) {
        Serial.println("Failed to create service: " + serviceUUID);
        return;
    }
    services[serviceUUID] = pService;
    Serial.println("Added service: " + serviceUUID);
}

void BoomBikeBLE::addCharacteristic(String serviceUUID, String CharUUID, String value, uint32_t properties, uint16_t max_len) {
    // Ensure the service exists, if not, create it
    if (services.find(serviceUUID) == services.end()) {
        addService(serviceUUID);
    }
    NimBLEService* pService = services[serviceUUID];
    if (pService == nullptr) {
        Serial.println("Service pointer is null after addService for: " + serviceUUID);
        return;
    }
    // Check if characteristic already exists
    NimBLECharacteristic* existingChar = pService->getCharacteristic(CharUUID.c_str());
    if (existingChar != nullptr) {
        Serial.println("Characteristic already exists: " + CharUUID);
        return;
    }
    // Create and add the characteristic
    NimBLECharacteristic* pCharacteristic = nullptr;
    if (pService) pCharacteristic = pService->createCharacteristic(CharUUID.c_str(), properties, max_len);
    if (!pCharacteristic) {
        Serial.println("Failed to create characteristic: " + CharUUID + " for service: " + serviceUUID);
        return;
    }
    if (value.length() > 0) {
        pCharacteristic->setValue(value.c_str());
    }
    Serial.println("Added characteristic: " + CharUUID + " to service: " + serviceUUID);
}

void BoomBikeBLE::setCharacteristicValue(String serviceUUID, String CharUUID, String value) {
    // Ensure the service exists
    if (services.find(serviceUUID) == services.end()) {
        addService(serviceUUID);
    }
    NimBLEService* pService = services[serviceUUID];
    if (pService == nullptr) {
        Serial.println("Service pointer is null in setCharacteristicValue for: " + serviceUUID);
        return;
    }
    // Ensure the characteristic exists
    NimBLECharacteristic* pCharacteristic = pService->getCharacteristic(CharUUID.c_str());
    if (pCharacteristic == nullptr) {
        // Try creating the characteristic directly here with default properties
        pCharacteristic = pService->createCharacteristic(CharUUID.c_str(), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, BLE_ATT_ATTR_MAX_LEN);
        if (!pCharacteristic) {
            Serial.println("Failed to create or locate characteristic: " + CharUUID);
            return;
        }
    }
    // Set the characteristic value
    pCharacteristic->setValue(value.c_str());
    Serial.println("Set characteristic value: " + CharUUID + " to: " + value);
    // If this characteristic has the NOTIFY property, attempt to notify subscribed centrals of the new value
    if (pCharacteristic->getProperties() & NIMBLE_PROPERTY::NOTIFY) {
        pCharacteristic->notify();
    }
}

void BoomBikeBLE::startAdvertising() {
    // Get the advertising object depending on the protocol (legacy or extended)
    #ifdef CONFIG_BT_NIMBLE_EXT_ADV // Use extended advertising
    // Create an extended advertisement instance with coded PHY for longer range and 1M PHY as fallback for compatibility
    NimBLEExtAdvertisement extAdv(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_1M);
    extAdv.setConnectable(true);
    // Start each service
    for (const auto& service : services) {
        service.second->start();
    }
    // Set the value of the advertising data for each service UUID
    extAdv.setServiceData(NimBLEUUID("AAAA"), std::string("Service Data Example")); // Example service data
    // Get pointer to the extended advertising instance
    NimBLEExtAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    if (pAdvertising->setInstanceData(0, extAdv)) { // Use instance ID 0
        // Start advertising with no timeout and unlimited events
        if (pAdvertising->start(0, 0)) {
            Serial.println("Started extended advertising");
        } else {
            Serial.println("Failed to start advertising");
        }
    } else {
        Serial.println("Failed to register advertising data");
    }
    
    #else // Use legacy advertising
    NimBLEAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
    // Add all service UUIDs to the advertising data
    for (const auto& service : services) {
        service.second->start();
        pAdvertising->addServiceUUID(service.first.c_str());
    }
    // Set the device name if provided
    if (deviceName.length() > 0) {
        pAdvertising->setName(deviceName.c_str());
    }
    // Start advertising
    pAdvertising->start();
    Serial.println("BLE advertising started with device name: " + deviceName);
    #endif
}

void BoomBikeBLE::verboseScanForDevices(int durationMillis) {
    NimBLEScan* pScan = NimBLEDevice::getScan(); // Create new scan
    pScan->setScanCallbacks(scanCallbacks); // Set callback to process each found device
    pScan->setActiveScan(true); // Active scan to get more data
    pScan->setPhy(scanPhy); // Set PHY for scanning
    Serial.println("Starting BLE scan for " + String(durationMillis) + " milliseconds...");
    pScan->start(durationMillis, false); // Start scan for specified duration
}

NimBLEAdvertisedDevice* BoomBikeBLE::findDeviceWithService(const NimBLEUUID& serviceUUID, int scanDuration) {
    NimBLEAdvertisedDevice* result = nullptr;
    NimBLEScan* pScan = NimBLEDevice::getScan(); // Create new scan
    NimBLEScanResults results = pScan->getResults(scanDuration * 1000);
    for (int i = 0; i < results.getCount(); i++) {
        const NimBLEAdvertisedDevice *device = results.getDevice(i);
        if (device->isAdvertisingService(serviceUUID)) {
            result = new NimBLEAdvertisedDevice(*device); // Return a copy of the found device
            break;
        }
    }
    pScan->clearResults(); // Clear results to free memory
    return result; // Return the found device or nullptr
}

bool BoomBikeBLE::connectToDevice(NimBLEAdvertisedDevice* device) {
    if (device == nullptr) {
        Serial.println("Cannot connect: device is null");
        return false;
    }
    // Create client if it doesn't exist
    if (pClient == nullptr) {
        pClient = NimBLEDevice::createClient();
    }
    // Connect to the device
    if (!pClient->isConnected()) {
        if (pClient->connect(device)) {
            Serial.print("Connected to device: ");
            Serial.println(device->getName().c_str());
            return true;
        } else {
            Serial.println("Failed to connect to device");
            return false;
        }
    } else {
        Serial.println("Already connected to a device");
        return true;
    }
}

void BoomBikeBLE::disconnect() {
    if (pClient && pClient->isConnected()) {
        pClient->disconnect();
        Serial.println("Disconnected from device");
    }
}

String ScanCallbacks::phyToString(uint8_t phy) {
    switch (phy) {
    case BLE_HCI_LE_PHY_1M:   return "1M";
    case BLE_HCI_LE_PHY_2M:   return "2M";
    case BLE_HCI_LE_PHY_CODED:return "CODED";
    default:                  return "UNKNOWN";
    }
}
