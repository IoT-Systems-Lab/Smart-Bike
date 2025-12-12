#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ICM-20948_ArduinoLibrary.h>

// ================= INSTELLINGEN =================
#define SDA_PIN 2
#define SCL_PIN 3

// AD0 Waarde: 
// 1 = Standaard SparkFun (Jumper Open)
// 0 = Alternatief (Jumper Dicht) of sommige Chinese clones
#define AD0_VAL 1  

ICM_20948_I2C myICM; 

// ================= HULPFUNCTIES VOORAF DEFINIEREN =================
void printFormattedFloat(float val, uint8_t leading, uint8_t decimals);
void printScaledAGMT(ICM_20948_I2C *sensor);

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  delay(1000); // Wacht even op USB connectie
  
  Serial.println("\nStart ICM-20948 Test...");

  // CRUCIAAL: Start I2C op de juiste pinnen
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); // Start op 100kHz voor stabiliteit

  bool initialized = false;
  while (!initialized) {
    Serial.print("Verbinden met IMU op adres-bit: ");
    Serial.println(AD0_VAL);

    myICM.begin(Wire, AD0_VAL);

    Serial.print("Sensor status: ");
    Serial.println(myICM.statusString());

    if (myICM.status != ICM_20948_Stat_Ok) {
      Serial.println("FOUT: Sensor niet gevonden.");
      Serial.println(" -> Check of AD0_VAL op 0 of 1 moet staan.");
      Serial.println(" -> Check bedrading (SDA=Pin2, SCL=Pin3).");
      delay(2000);
    } else {
      Serial.println("SUCCESS: Sensor is online!");
      initialized = true;
    }
  }
}

// ================= LOOP =================
void loop() {
  if (myICM.dataReady()) {
    myICM.getAGMT(); // Haal nieuwe data op
    printScaledAGMT(&myICM); // Print data netjes
    delay(50); // Niet te snel spammen
  } else {
    // Serial.println("Wachten op data...");
    delay(10);
  }
}

// ================= PRINT FUNCTIES =================

void printFormattedFloat(float val, uint8_t leading, uint8_t decimals) {
  float aval = abs(val);
  if (val < 0) {
    Serial.print("-");
  } else {
    Serial.print(" ");
  }
  for (uint8_t indi = 0; indi < leading; indi++) {
    uint32_t tenpow = 0;
    if (indi < (leading - 1)) {
      tenpow = 1;
    }
    for (uint8_t c = 0; c < (leading - 1 - indi); c++) {
      tenpow *= 10;
    }
    if (aval < tenpow) {
      Serial.print("0");
    } else {
      break;
    }
  }
  if (val < 0) {
    Serial.print(-val, decimals);
  } else {
    Serial.print(val, decimals);
  }
}

void printScaledAGMT(ICM_20948_I2C *sensor) {
  Serial.print("Acc (mg) [ ");
  printFormattedFloat(sensor->accX(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->accY(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->accZ(), 5, 2);
  Serial.print(" ], Gyr (DPS) [ ");
  printFormattedFloat(sensor->gyrX(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->gyrY(), 5, 2);
  Serial.print(", ");
  printFormattedFloat(sensor->gyrZ(), 5, 2);
  Serial.print(" ], Tmp (C) [ ");
  printFormattedFloat(sensor->temp(), 5, 2);
  Serial.print(" ]");
  Serial.println();
}