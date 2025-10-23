#include "DFRobot_GNSSAndRTC.h"

// Gebruik I2C-communicatie
#define I2C_COMMUNICATION

#ifdef I2C_COMMUNICATION

// Maak GNSS object via I2C
DFRobot_GNSSAndRTC_I2C gnss(&Wire, MODULE_I2C_ADDRESS);

#endif

void setup() {
  Serial.begin(115200);
  delay(2000);
  Serial.println("Start DFRobot GNSS via I2C...");

  // ESP32: SDA = GPIO21, SCL = GPIO22 standaard
  Wire.begin(21, 22);

  // Wachten tot module gevonden wordt
  while (!gnss.begin()) {
    Serial.println("Geen GNSS-module gevonden, controleer verbinding...");
    delay(1000);
  }

  Serial.println("GNSS-module gevonden!");
  gnss.enablePower();

  // Zet welke GNSS-systemen gebruikt worden
  gnss.setGnss(gnss.eGPS);
}

void loop() {
  // Lees GNSS-data
  DFRobot_GNSSAndRTC::sTim_t utc = gnss.getUTC();
  DFRobot_GNSSAndRTC::sTim_t date = gnss.getDate();
  DFRobot_GNSSAndRTC::sLonLat_t lat = gnss.getLat();
  DFRobot_GNSSAndRTC::sLonLat_t lon = gnss.getLon();
  double high = gnss.getAlt();
  uint8_t starUsed = gnss.getNumSatUsed();
  double sog = gnss.getSog();
  double cog = gnss.getCog();
  
  Serial.println("==================================");
  Serial.printf("Satellieten: %d\n", gnss.getNumSatUsed());
  Serial.printf("%04d/%02d/%02d %02d:%02d:%02d\n", 
    date.year, date.month, date.date, utc.hour, utc.minute, utc.second);

  Serial.printf("Lat: %.6f° %c\n", lat.latitudeDegree, (char)lat.latDirection);
  Serial.printf("Lon: %.6f° %c\n", lon.lonitudeDegree, (char)lon.lonDirection);
  Serial.printf("Satellieten: %d\n", starUsed);
  Serial.printf("Hoogte: %.2f m\n", high);
  Serial.printf("Snelheid (SOG): %.2f kn\n", sog);
  Serial.printf("Koers (COG): %.2f°\n", cog);
  Serial.printf("GNSS Mode: %d\n", gnss.getGnssMode());

  delay(1000);
}


