// Simple dummy node to test BLE Long Range communication
// The device will periodically advertise itself with some data over BLE Long Range (Coded PHY)
// A BoomBike Gateway (running separate code) can scan for these advertisements and forward the data to InfluxDB

//#include "BoomBikeBLE.h"
#include <Arduino.h>
#include <NimBLEDevice.h>
#include "esp_sleep.h"

// BLE communication
bool deviceConnected = false;
#define SERVICE_UUID "ABCD"
#define CHARACTERISTIC_UUID "1234"
#define LED_PIN 10 // Onboard LED pin

static uint32_t advTime = 5000; // Advertising time in milliseconds
static uint32_t sleepSeconds = 1; // Time to sleep between advertisements

class ServerCallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) override {
    Serial.printf("Client connected: %s\n", connInfo.getAddress().toString().c_str());
    deviceConnected = true;
    digitalWrite(LED_PIN, HIGH);
  }
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) override {
    Serial.printf("Client disconnected - sleeping for %" PRIu32 " seconds\n", sleepSeconds);
    deviceConnected = false;
    digitalWrite(LED_PIN, LOW);
    esp_deep_sleep_start();
  }
} serverCallbacks;

class AdvertisingCallbacks : public NimBLEExtAdvertisingCallbacks {
  void onStopped(NimBLEExtAdvertising* pAdv, int reason, uint8_t instId) override {
    // Check the reason adervtising stopped, don't sleep if client is connecting
    Serial.printf("Advertising instance %u stopped\n", instId);
    switch (reason) {
      case 0:
        Serial.printf("Client connecting\n");
        return;
      case BLE_HS_ETIMEOUT:
        Serial.printf("Time expired - sleeping for %" PRIu32 " seconds\n", sleepSeconds);
        break;
      default:
        break;
    }
    esp_deep_sleep_start();
  }
} advertisingCallbacks;


void setup() {
  Serial.begin(115200);
  Serial.println("Starting Dummy Node...");

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  BLEDevice::init("DummyNode");

  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(&serverCallbacks);

  NimBLEService* pService = pServer->createService(SERVICE_UUID);
  NimBLECharacteristic* pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
  
  pCharacteristic->setValue("Hello World");

  pService->start();

  NimBLEExtAdvertisement extAdv(BLE_HCI_LE_PHY_CODED, BLE_HCI_LE_PHY_CODED);

  extAdv.setConnectable(true);
  extAdv.setScannable(false); // per BLE spec, connectable adv cannot be scannable
  extAdv.setName("DummyNode");
  extAdv.setServiceData(NimBLEUUID(SERVICE_UUID),
                        std::string("BoomBikeDummy Node to transmit data over Long Range :)"));
  extAdv.addServiceUUID(SERVICE_UUID); // include service UUID in advertisement

  NimBLEExtAdvertising* pAdvertising = NimBLEDevice::getAdvertising();
  pAdvertising->setCallbacks(&advertisingCallbacks);

  if (pAdvertising->setInstanceData(0, extAdv)) {
    if (pAdvertising->start(0, advTime)) {
      Serial.printf("Started advertising\n");
    } else {
      Serial.printf("Failed to start advertising\n");
    }
  } else {
    Serial.printf("Failed to register advertisement data\n");
  }

  esp_sleep_enable_timer_wakeup(sleepSeconds * 1000000);
}

void loop() {
}
