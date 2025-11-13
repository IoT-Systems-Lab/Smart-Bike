// Simple dummy gateway to test BLE Long Range communication
// The device will scan for Dummy Nodes advertising over BLE Long Range (Coded PHY)
// and connect to them to read a characteristic value

#include <Arduino.h>
#include <NimBLEDevice.h>

#define SERVICE_UUID "ABCD"
#define CHARACTERISTIC_UUID "1234"

static const NimBLEAdvertisedDevice* advDevice;
static bool doConnect = false;
static uint32_t scanTime = 10 * 1000; // Scan for 10 seconds

// Class to handle callbacks for client connection events
class ClientCallbacks : public NimBLEClientCallbacks {
  void onConnect(NimBLEClient* pClient) override {
    Serial.printf("Connected to BLE device\n");
  }
  void onDisconnect(NimBLEClient* pClient, int reason) override {
    Serial.printf("%s Disconnected, reason = %d - Starting scan\n", pClient->getPeerAddress().toString().c_str(), reason);
    NimBLEDevice::getScan()->start(scanTime);
  }
} clientCallbacks;

// Class to handle callbacks when advertisements are received
class scanCallbacks : public NimBLEScanCallbacks {
  void onResult(const NimBLEAdvertisedDevice* advertisedDevice) override {
    Serial.printf("Advertised Device found: %s\n", advertisedDevice->toString().c_str());
    if (advertisedDevice->isAdvertisingService(NimBLEUUID(SERVICE_UUID))) {
      Serial.printf("Found our service\n");
      doConnect = true;
      advDevice = advertisedDevice;
      NimBLEDevice::getScan()->stop();
    }
  }
  void onScanEnd(const NimBLEScanResults& results, int rc) override {
    Serial.printf("Scan Ended\n");
  }
} scanCallbacks;

// Handles provisioning of clients and interfaces with the server
bool connectToServer() {
  NimBLEClient* pClient = nullptr;

  pClient = NimBLEDevice::createClient();
  pClient->setClientCallbacks(&clientCallbacks, false);

  pClient->setConnectPhy(BLE_GAP_LE_PHY_CODED_MASK);
  pClient->setConnectTimeout(5*1000);

  if (!pClient->connect(advDevice)) {
    // Created a client but failed to connect, don't keep it
    NimBLEDevice::deleteClient(pClient);
    Serial.println("Failed to connect, deleted client\n");
    return false;
  }

  Serial.printf("Connected to: %s RSSI: %d\n", pClient->getPeerAddress().toString().c_str(), pClient->getRssi());

  // Read/Write characteristics can be added here
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService(SERVICE_UUID);
  if (pSvc) {
    pChr = pSvc->getCharacteristic(CHARACTERISTIC_UUID);
    if (pChr) {
      if (pChr->canRead()) {
        std::string value = pChr->readValue();
        Serial.printf("Characteristic value: %s\n", value.c_str());
      }
    }
  } else {
    Serial.printf("Failed to find our service UUID: %s\n", SERVICE_UUID);
  }

  NimBLEDevice::deleteClient(pClient);
  Serial.printf("Read and disconnected from the server\n");
  return true;
}

void setup() {
  Serial.begin(115200);
  Serial.printf("Starting BLE Client\n");

  NimBLEDevice::init("GatewayClient");

  NimBLEScan* pScan = NimBLEDevice::getScan();
  pScan->setScanCallbacks(&scanCallbacks);

  pScan->setInterval(45); // how often to scan (in ms)
  pScan->setWindow(15);   // how long to scan (in ms)
  pScan->setActiveScan(true); // active scan uses more power, but get results faster

  pScan->start(scanTime);
  Serial.println("Scan started");
}

void loop() {
  // If we found a device to connect to, do it now
  if (doConnect) {
    Serial.println("Connecting to server...");
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("Failed to connect to the server; restarting scan.");
    }
    doConnect = false;
    NimBLEDevice::getScan()->start(scanTime);
  }

  delay(10); 
}