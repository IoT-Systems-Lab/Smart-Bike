#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>
#include <time.h>

#include "BoomBikeInfluxPublisher.h"
#include "BoomBikeBLE.h"
#include "BoomBikeUltrasonic.h"
#include "secrets.h"   // bevat WiFi + InfluxDB credentials

// ----------------------
// Pinconfiguratie
// ----------------------
#define SDA_PIN 5
#define SCL_PIN 6
#define TRIG_PIN 3
#define ECHO_PIN 4
#define BME680_ADDRESS 0x76
#define LED_PIN 10
#define BUTTON_PIN 7
#define SERVICE_UUID "92faf2e0-05f9-491b-afeb-8c913697b403"
#define CHARACTERISTIC_UUID "7ae4c208-bd43-41bd-86fc-e505ec17929d"

// ----------------------
// Globale objecten
// ----------------------
Adafruit_BME680 bme;
// Gebruik de nieuwe constructor voor de BLE client
BoomBikeBLE bikeBLE(SERVICE_UUID, CHARACTERISTIC_UUID);
BoomBikeUltrasonic ultrasonic(TRIG_PIN, ECHO_PIN);
BoomBikeInfluxPublisher influxPublisher(
  WIFI_SSID, WIFI_PASSWORD,
  INFLUXDB_URL, INFLUXDB_ORG,
  INFLUXDB_BUCKET, INFLUXDB_TOKEN,
  "CET-1"
);
// Set timezone to Central European Time (CET, UTC+1).
// POSIX TZ string: name and offset (note sign convention). "CET-1" corresponds to UTC+1.

// Timing and accumulation state (file scope so helper functions can use them)
static const unsigned long SAMPLE_INTERVAL_MS = 500;   // print / sample every 500 ms
static const unsigned long PUBLISH_INTERVAL_MS = 6000; // publish averages every 6000 ms

static unsigned long lastSampleMillis = 0;
static unsigned long lastPublishMillis = 0;

// accumulation for averaging
static unsigned long sampleCount = 0;
static double sumTemp = 0.0;
static double sumHum = 0.0;
static double sumPress = 0.0;
static double sumIAQ = 0.0;
static double sumDistance = 0.0;
// pass detection accumulators
static const int MAX_PASSES = 50;
static int passIndex = 0;
static String passTimes[MAX_PASSES];

// Helper forward declarations
String formatTimeNow(unsigned long now);
void sampleIfDue(unsigned long now);
void publishIfDue(unsigned long now);
void resetAccumulators();

// ----------------------
// Bluetooth Dataverwerking
// ----------------------

// Callback functie voor wanneer BLE data wordt ontvangen
void onBleDataReceived(const std::string& address, const std::string& data) {
  Serial.print("--- BLE Data Ontvangen ---\n");
  Serial.print("Van apparaat: ");
  Serial.println(address.c_str());
  Serial.print("Waarde: ");
  Serial.println(data.c_str());
  Serial.print("--------------------------\n");

  // data formaat:
  // 'accX,accY,accZ,gyroX,gyroY,gyroZ,tempBike,latDirection,longDirection,latitude,longitude,speed'
  // alle waarden moeten gedeeld worden door 100, latitude en longitude zijn in 1e graads notatie (bijv. 51060148 voor 51.060148)
  float accX = 0.0, accY = 0.0, accZ = 0.0;
  float gyroX = 0.0, gyroY = 0.0, gyroZ = 0.0;
  float tempBike = 0.0;
  float latitude = 0.0, longitude = 0.0;
  char latDirection = 'N', longDirection = 'E';
  float speed = 0.0;

  int parsed = sscanf(data.c_str(), "%f,%f,%f,%f,%f,%f,%f,%c,%c,%f,%f,%f",
                      &accX, &accY, &accZ,
                      &gyroX, &gyroY, &gyroZ,
                      &tempBike,
                      &latDirection, &longDirection,
                      &latitude, &longitude, &speed);
  
  // nu accelero delen door 100000 (omdat in milli-g wordt gestuurd), gyro door 100
  accX /= 100000.0; accY /= 100000.0; accZ /= 100000.0;
  gyroX /= 100.0; gyroY /= 100.0; gyroZ /= 100.0;
  tempBike /= 100.0;
  speed /= 100.0;
  // latitude en longitude delen door 1e6
  latitude /= 1000000.0; longitude /= 1000000.0;
  // aanpassen op N/S en E/W
  if (latDirection == 'S') latitude = -latitude;
  if (longDirection == 'W') longitude = -longitude;

  // nu de waarden toevoegen aan InfluxDB publisher
  influxPublisher.addData("accX", accX);
  influxPublisher.addData("accY", accY);
  influxPublisher.addData("accZ", accZ);
  influxPublisher.addData("gyroX", gyroX);
  influxPublisher.addData("gyroY", gyroY);
  influxPublisher.addData("gyroZ", gyroZ);
  influxPublisher.addData("tempBike", tempBike);
  influxPublisher.addData("latitude", latitude);
  influxPublisher.addData("longitude", longitude);
  influxPublisher.addData("speed", speed);

  // data wordt gepubliceerd in de main loop
  // influxPublisher.publishData();
}

// ----------------------
// Setup
// ----------------------
void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("BoomBike Gateway + BME680 gestart");

  // WiFi + InfluxDB + BLE
  influxPublisher.begin();
  // enable ultrasonic pass detection
  ultrasonic.enablePassDetection();
  // Initialiseer de BLE client en registreer de callback
  bikeBLE.begin();
  bikeBLE.onDataReceived(onBleDataReceived);

  // I2C en BME680 initialiseren
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!bme.begin(BME680_ADDRESS, &Wire)) {
    Serial.println("Geen BME680 sensor gevonden!");
    //while (1) delay(10);
  }
  Serial.println("BME680 sensor gevonden");

  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320°C voor 150 ms
}


// ----------------------
// Helper implementations
// ----------------------
String formatTimeNow(unsigned long now) {
  // Return HH:MM:SS using local time if available, otherwise uptime-based time
  time_t t = time(nullptr);
  char timeBuf[16] = {0};
  if (t != 0) {
    struct tm *lt = localtime(&t);
    if (lt) {
      snprintf(timeBuf, sizeof(timeBuf), "%02d:%02d:%02d", lt->tm_hour, lt->tm_min, lt->tm_sec);
      return String(timeBuf);
    }
  }
  unsigned long s = now / 1000UL;
  unsigned int hh = (s / 3600UL) % 24UL;
  unsigned int mm = (s / 60UL) % 60UL;
  unsigned int ss = s % 60UL;
  snprintf(timeBuf, sizeof(timeBuf), "%02u:%02u:%02u", hh, mm, ss);
  return String(timeBuf);
}

void sampleIfDue(unsigned long now) {
  if (now - lastSampleMillis < SAMPLE_INTERVAL_MS) return;
  lastSampleMillis = now;

  // BME680 reading
  bool bmeOk = bme.performReading();
  float temp = NAN;
  float hum = NAN;
  float press = NAN;
  float iaq = NAN;
  if (bmeOk) {
    temp = bme.temperature;
    hum = bme.humidity;
    press = bme.pressure / 100.0; // hPa
    float gas = bme.gas_resistance;
    const float gas_lower = 10000.0;
    const float gas_upper = 1000000.0;
    iaq = 0.4 * 500.0 * (1.0 - ((log(gas) - log(gas_lower)) / (log(gas_upper) - log(gas_lower))));
    iaq = constrain(iaq, 0.0, 500.0);
  } else {
    Serial.println("FOUT bij BME680 meting");
  }

  // Ultrasonic reading
  float distance = ultrasonic.readDistance();
  bool validDist = (distance > 0.0 && distance < 400.0);
  bool passed = ultrasonic.checkForPass();
  if (passed) {
    String tstr = formatTimeNow(now);
    if (passIndex < MAX_PASSES) {
      passTimes[passIndex++] = tstr;
    } else {
      Serial.println("Pass times array full, ignoring extra pass");
    }
    Serial.print(" PASS_TIME:"); Serial.print(tstr);
  }

  // Print current values to Serial (every sample)
  Serial.print("Sample: ");
  if (!isnan(temp)) {
    Serial.print("Temp:"); Serial.print(temp); Serial.print(" °C ");
  } else {
    Serial.print("Temp:NaN ");
  }
  if (!isnan(hum)) {
    Serial.print("Hum:"); Serial.print(hum); Serial.print("% ");
  }
  if (!isnan(press)) {
    Serial.print("Press:"); Serial.print(press); Serial.print(" hPa ");
  }
  if (!isnan(iaq)) {
    Serial.print("AQI:"); Serial.print(iaq); Serial.print(" ");
  }
  Serial.print("Distance:"); Serial.print(distance); Serial.print(" cm");
  if (validDist) {
    Serial.print(" Pass:"); Serial.print(passed ? "YES" : "NO");
  }
  Serial.println();

  // Accumulate valid readings for averaging
  if (!isnan(temp)) { sumTemp += temp; }
  if (!isnan(hum))  { sumHum += hum; }
  if (!isnan(press)){ sumPress += press; }
  if (!isnan(iaq))  { sumIAQ += iaq; }
  if (validDist) { sumDistance += distance; }

  // count sample for publish averaging
  sampleCount++;
}

void publishIfDue(unsigned long now) {
  if (now - lastPublishMillis < PUBLISH_INTERVAL_MS) return;
  lastPublishMillis = now;

  if (sampleCount == 0) {
    Serial.println("No samples collected in interval, skipping publish.");
    return;
  }

  double avgTemp = sumTemp / sampleCount;
  double avgHum = sumHum / sampleCount;
  double avgPress = sumPress / sampleCount;
  double avgIAQ = sumIAQ / sampleCount;
  double avgDistance = sumDistance / sampleCount;

  Serial.println("Publishing averages:");
  Serial.print("Avg Temp: "); Serial.println(avgTemp);
  Serial.print("Avg Hum: "); Serial.println(avgHum);
  Serial.print("Avg Press: "); Serial.println(avgPress);
  Serial.print("Avg AQI: "); Serial.println(avgIAQ);
  Serial.print("Avg Distance: "); Serial.println(avgDistance);

  // Send averaged values once
  influxPublisher.addData("Temperature_C", (float)avgTemp);
  influxPublisher.addData("Humidity_Percent", (int)round(avgHum));
  influxPublisher.addData("Pressure_hPa", (float)avgPress);
  influxPublisher.addData("AirQualityIndex", (int)round(avgIAQ));
  influxPublisher.addData("Distance_cm", (float)avgDistance);
  // Include pass info collected during the interval (single field)
  influxPublisher.addData("pass_count", (long)passIndex);
  if (passIndex > 0) {
    Serial.print("==> PASS INDEX:");
    Serial.println(passIndex);
    // Send all pass times as separate fields with the same key (library will handle overwriting if necessary).
    // According to request, add each time using the same key name.
    for (int i = 0; i < passIndex; i++) {
      influxPublisher.addData("Finish time:", passTimes[i].c_str());
    }
  }
  influxPublisher.publishData();

  resetAccumulators();
}

void resetAccumulators() {
  sampleCount = 0;
  sumTemp = sumHum = sumPress = sumIAQ = sumDistance = 0.0;
  passIndex = 0;
  for (int i = 0; i < MAX_PASSES; i++) passTimes[i] = "";
}

// ----------------------
// Loop
// ----------------------
void loop() {
  unsigned long now = millis();
  sampleIfDue(now);
  bikeBLE.loop(); // Call the BLE loop to handle scan if needed
  publishIfDue(now);


  // short yield to avoid starving background tasks
  delay(1);
}
