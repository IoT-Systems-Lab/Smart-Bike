#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include "BoomBikeInfluxPublisher.h"
#include "BoomBikeBLE.h"
#include "BoomBikeUltrasonic.h"
#include "secrets.h"   // bevat jouw WiFi + InfluxDB credentials

// ----------------------
// Pinconfiguratie
// ----------------------
#define SDA_PIN 5
#define SCL_PIN 6
#define TRIG_PIN 3
#define ECHO_PIN 9
#define BME680_ADDRESS 0x76

// ----------------------
// Globale objecten
// ----------------------
Adafruit_BME680 bme;
BoomBikeBLE bikeBLE("BoomBike-Gateway");
BoomBikeUltrasonic ultrasonic(TRIG_PIN, ECHO_PIN);
BoomBikeInfluxPublisher influxPublisher(
  WIFI_SSID, WIFI_PASSWORD,
  INFLUXDB_URL, INFLUXDB_ORG,
  INFLUXDB_BUCKET, INFLUXDB_TOKEN
);

// ----------------------
// Setup
// ----------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BoomBike Gateway + BME680 gestart");

  // WiFi + InfluxDB + BLE
  bikeBLE.begin();
  bikeBLE.setPhy(NimBLEScan::Phy::SCAN_CODED);
  influxPublisher.begin();

  // I2C en BME680 initialiseren
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(BME680_ADDRESS, &Wire)) {
    Serial.println("Geen BME680 sensor gevonden!");
    while (1) delay(10);
  }
  Serial.println("BME680 sensor gevonden");

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C voor 150 ms
}

// ----------------------
// Loop
// ----------------------
void loop() {
  // --- BME680 uitlezen ---
  if (bme.performReading()) {
    float temp = bme.temperature;
    float hum = bme.humidity;
    float press = bme.pressure / 100.0; // hPa
    float gas = bme.gas_resistance; // in ohms
    float gas_lower = 10000.0;      // "clean air" lower bound
    float gas_upper = 1000000.0;    // "very polluted" upper bound

    // logarithmic mapping to 0-500 IAQ
    float iaq = 500.0 * (1.0 - ((log(gas) - log(gas_lower)) / (log(gas_upper) - log(gas_lower))));

    // Constrain to 0-500
    iaq = constrain(iaq, 0.0, 500.0);

    Serial.print("Temp: "); Serial.print(temp);
    Serial.print(" °C, Hum: "); Serial.print(hum);
    Serial.print(" %, Press: "); Serial.print(press);
    Serial.print(" hPa, AQI: "); Serial.print(iaq);
    Serial.print("\n");

    // --- InfluxDB: stuur echte sensordata ---
    influxPublisher.addData("Temperature_C", temp);
    influxPublisher.addData("Humidity_Percent", int(hum));
    influxPublisher.addData("Pressure_hPa", press);
    influxPublisher.addData("AirQualityIndex", int(iaq));
    influxPublisher.publishData();
  } else {
    Serial.println("FOUT bij BME680 meting");
  }

  // --- HC-SR04 uitlezen ---
  float distance = ultrasonic.readDistance();
  Serial.print("Afstand: ");
  Serial.print(distance);
  Serial.println(" cm");

  influxPublisher.addData("Distance_cm", distance);
  influxPublisher.publishData();

  // --- Dummy GPS-gegevens (optioneel) ---
  influxPublisher.addData("latitude", 51.060148f);
  influxPublisher.addData("longitude", 3.707525f);
  influxPublisher.publishData();

  delay(5000); // elke 5 seconden nieuwe upload
}
