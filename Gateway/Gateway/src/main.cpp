// BoomBike Gateway
// Receive data from the bike over BLE
// Send data to InfluxDB over WiFi

#include "BoomBikeInfluxPublisher.h"
#include "BoomBikeUltrasonic.h"
#include "BoomBikeBLE.h"
#include <Arduino.h>
// Include local secrets (create src/secrets.h from src/secrets.h.example and keep it out of git)
#include "secrets.h"

// Network credentials (defined in src/secrets.h - not on git to avoid committing real secrets)
const char* ssid = WIFI_SSID;
const char* password = WIFI_PASSWORD;

// InfluxDB credentials
const char* influxdb_url = INFLUXDB_URL;
const char* influxdb_org = INFLUXDB_ORG;
const char* influxdb_bucket = INFLUXDB_BUCKET;
const char* influxdb_token = INFLUXDB_TOKEN;

// InfluxDB publisher
BoomBikeInfluxPublisher influxPublisher(ssid, password, influxdb_url, influxdb_org, influxdb_bucket, influxdb_token);

// BLE communication
BoomBikeBLE bikeBLE("BoomBike-Gateway");

// Ultrasonic sensor on pins 8 (trigger) and 9 (echo)
BoomBikeUltrasonic ultrasonic(8, 9);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike Gateway...");

  bikeBLE.begin();
  bikeBLE.setPhy(NimBLEScan::Phy::SCAN_CODED);

  influxPublisher.begin();
}

void loop() {
//   NimBLEAdvertisedDevice* device = bikeBLE.findDeviceWithService(NimBLEUUID("abcd"), 3); // Scan for 3 seconds to find the bike service
//   if (device) {
//     Serial.print("Found device: ");
//     Serial.println(device->toString().c_str());
//   } else {
//     Serial.println("Device not found, retrying scan...");
//     return; // Retry scanning
//   }
//   bikeBLE.connectToDevice(device);
//   delay(4000); // Stay connected for 4 seconds
//   bikeBLE.disconnect();
//   delay(1000); // Wait 1 second before scanning again
  
  // Read distance from ultrasonic sensor
  //float distance = ultrasonic.readDistance();
  //Serial.print("Ultrasonic distance: ");
  //Serial.println(distance);

  // Publish data to InfluxDB
  //influxPublisher.addData("uptime_seconds", millis() / 1000);
  //influxPublisher.addData("distance", distance);
  //influxPublisher.publishData();

  // Send dummy GPS coordinates (for Grafana Geomap testing)
  // Example coordinates: New York City (latitude, longitude)
  influxPublisher.addData("latitude", 51.060148f);
  influxPublisher.addData("longitude", 3.707525f);
  influxPublisher.publishData();
}
