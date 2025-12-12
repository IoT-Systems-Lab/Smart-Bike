#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ICM-20948_ArduinoLibrary.h> 
#include <NimBLEDevice.h>
#include "DFRobot_GNSSAndRTC.h"

// ================= CONSTANTEN & PINS =================
#define SDA_PIN 2 //D0
#define SCL_PIN 3 //D1
#define AD0_VAL 1  
const byte HALL_PIN = 4; //D3

// Sensor objecten
ICM_20948_I2C myICM;
DFRobot_GNSSAndRTC_I2C gnss(&Wire, MODULE_I2C_ADDRESS);
bool gnssOnline = false;

// Hall Variabelen
volatile unsigned long pulseCount = 0;
const float WHEEL_CIRC_M = 2.10;   
float currentKmh = 0.0; 

// Reset Logic Variabele
int failedConnectCount = 0;     
const int MAX_FAILED_ATTEMPTS = 10; 

// Interrupt Routine
void IRAM_ATTR hallISR() {
  pulseCount++;
}

// ================= BLE CONFIG =================
#define SERVICE_UUID        "ABCD"
#define CHARACTERISTIC_UUID "1234"
NimBLECharacteristic* pCharacteristic = nullptr;
NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

// ================= STATE MACHINE CONFIG =================
enum SystemState {
  STATE_HALL_MEASURE,   // 2 seconden meten
  STATE_BLE_START,      // Start advertising
  STATE_BLE_WAIT        // Wacht op connectie of timeout
};

SystemState currentState = STATE_HALL_MEASURE;
unsigned long stateStartTime = 0;

const unsigned long HALL_MEASURE_TIME = 2000; // 2 seconden meten
const unsigned long BLE_TIMEOUT = 3000;       // Max 3 seconden proberen te verbinden

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  
  // 1. Hall Setup
  pinMode(HALL_PIN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(HALL_PIN), hallISR, FALLING); 

  // 2. I2C Setup
  Wire.begin(SDA_PIN, SCL_PIN);
  Wire.setClock(100000); 

  // 3. GNSS Init
  Serial.print("GPS Init... ");
  if (gnss.begin()) {
    Serial.println("OK");
    gnss.enablePower();
    gnss.setGnss(gnss.eGPS);
    gnssOnline = true; 
  } else {
    Serial.println("Niet gevonden");
  }

  // 4. IMU Init
  myICM.begin(Wire, AD0_VAL);
  if (myICM.status != ICM_20948_Stat_Ok) {
    Serial.println("IMU Fout!");
  } else {
    Serial.println("IMU OK");
    ICM_20948_fss_t myFSS;
    myFSS.a = gpm2; 
    myICM.setFullScale((ICM_20948_Internal_Acc | ICM_20948_Internal_Gyr), myFSS);
  }

  // 5. BLE Init
  NimBLEDevice::init("CodedNode");
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  pServer = NimBLEDevice::createServer();
  
  class ServerCallbacks : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer* pServer, NimBLEConnInfo& info) override {
      deviceConnected = true;
    }
    void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo&, int reason) override {
      deviceConnected = false;
    }
  };
  pServer->setCallbacks(new ServerCallbacks());

  NimBLEService* service = pServer->createService(SERVICE_UUID);
  pCharacteristic = service->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
                    );
  service->start();

  // Advertising configureren
  NimBLEExtAdvertising* adv = NimBLEDevice::getAdvertising();
  NimBLEExtAdvertisement extAdv;
  extAdv.setLegacyAdvertising(false);
  extAdv.setConnectable(true);
  extAdv.setScannable(false);
  extAdv.setPrimaryPhy(BLE_HCI_LE_PHY_CODED);
  extAdv.setSecondaryPhy(BLE_HCI_LE_PHY_CODED);
  extAdv.setCompleteServices(NimBLEUUID(SERVICE_UUID));
  extAdv.setName("CodedNode");
  adv->setInstanceData(0, extAdv);

  Serial.println("Setup Klaar. Start cyclus...");
  
  currentState = STATE_HALL_MEASURE;
  stateStartTime = millis();
  pulseCount = 0; 
  Serial.println("--- FASE 1: Meten (3s) ---");
}

// ================= LOOP =================
void loop() {
  unsigned long currentMillis = millis();

  switch (currentState) {
    
    // ---------------------------------------------------------
    // FASE 1: Meten
    // ---------------------------------------------------------
    case STATE_HALL_MEASURE:
      if (currentMillis - stateStartTime >= HALL_MEASURE_TIME) {
        
        noInterrupts();
        unsigned long pulses = pulseCount;
        interrupts();

        float rps = (float)pulses / (HALL_MEASURE_TIME / 1000.0);
        float m_per_s = rps * WHEEL_CIRC_M;
        currentKmh = m_per_s * 3.6;

        Serial.printf("Meting: %lu pulsen, %.1f km/h\n", pulses, currentKmh);

        currentState = STATE_BLE_START;
      }
      break;

    // ---------------------------------------------------------
    // FASE 2: BLE Start
    // ---------------------------------------------------------
    case STATE_BLE_START:
      Serial.println("--- FASE 2: BLE Starten ---");
      NimBLEDevice::startAdvertising(0);
      stateStartTime = currentMillis; 
      currentState = STATE_BLE_WAIT;
      break;

    // ---------------------------------------------------------
    // FASE 3: Wachten
    // ---------------------------------------------------------
    case STATE_BLE_WAIT:
      
      //SUCCES (Verbonden)
      if (deviceConnected) {        
        failedConnectCount = 0;//reset

        myICM.getAGMT();
        
        char latDirection = 'N'; char lonDirection = 'E';//In geval van geen GPS
        long latIntDegree = 51060936; long lonIntDegree = 3708244;//In geval van geen GPS
        
        if (gnssOnline) {
             auto lat = gnss.getLat(); auto lon = gnss.getLon();
             if(lat.latDirection != 0) {
                 latDirection = lat.latDirection; lonDirection = lon.lonDirection;
                 latIntDegree = (long)(lat.latitudeDegree * 1000000.0);
                 lonIntDegree = (long)(lon.lonitudeDegree * 1000000.0);
             }
        }

        char buf[150];
        snprintf(buf, sizeof(buf), 
               "%d,%d,%d,%d,%d,%d,%d,%c,%c,%ld,%ld,%d", 
               (int)(myICM.accX()*100), (int)(myICM.accY()*100), (int)(myICM.accZ()*100), 
               (int)(myICM.gyrX()*100), (int)(myICM.gyrY()*100), (int)(myICM.gyrZ()*100), 
               (int)(myICM.temp()*100),
               latDirection, lonDirection, latIntDegree, lonIntDegree, 
               (int)(currentKmh * 100)); //alle data samenvoegen

        pCharacteristic->setValue(buf);
        pCharacteristic->notify();
        Serial.print("Verzonden: "); Serial.println(buf);
        
        delay(50); 
        pServer->disconnect(0);        
        Serial.println("--- FASE 1: Meten (2s) ---");
        pulseCount = 0; 
        stateStartTime = millis();
        currentState = STATE_HALL_MEASURE;
      }
      
      // OPTIE B: TIMEOUT (Fout)
      else if (currentMillis - stateStartTime >= BLE_TIMEOUT) {
        NimBLEDevice::stopAdvertising();
        
        // --- FOUTEN TELLEN ---
        failedConnectCount++;
        Serial.printf("Timeout #%d / %d\n", failedConnectCount, MAX_FAILED_ATTEMPTS);

        // --- CHECK VOOR RESTART ---
        if (failedConnectCount >= MAX_FAILED_ATTEMPTS) {
          Serial.println("Te vaak mislukt. ESP32 wordt herstart..");
          delay(100);
          ESP.restart(); // HARDE RESET
        }
        
        Serial.println("--- FASE 1: Meten (3s) ---");
        pulseCount = 0; 
        stateStartTime = millis();
        currentState = STATE_HALL_MEASURE;
      }
      break;
  }
}