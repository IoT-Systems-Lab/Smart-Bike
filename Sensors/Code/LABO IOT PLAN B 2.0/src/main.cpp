#include <Arduino.h>
#include <Wire.h>
#include <SparkFun_ICM-20948_ArduinoLibrary.h> 
#include <NimBLEDevice.h>
#include "DFRobot_GNSSAndRTC.h"

// ================= CONSTANTEN & PINS =================
#define SDA_PIN 2 //D0
#define SCL_PIN 3 //D1
#define AD0_VAL 1  
const byte HALL_PIN = 4; //D2

// ================= GLOBALE VARIABELEN =================
bool Staatstil = false;
int stilstandTeller = 0;     
const int STILSTAND_LIMIET = 10; 

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
const int MAX_FAILED_ATTEMPTS = 11; 

// Interrupt Routine
void IRAM_ATTR hallISR() {
  pulseCount++;
}

// ================= BLE CONFIG =================
#define SERVICE_UUID        "92faf2e0-05f9-491b-afeb-8c913697b403"
#define CHARACTERISTIC_UUID "7ae4c208-bd43-41bd-86fc-e505ec17929d"
NimBLECharacteristic* pCharacteristic = nullptr;
NimBLEServer* pServer = nullptr;
bool deviceConnected = false;

// ================= STATE MACHINE CONFIG =================
enum SystemState {
  STATE_HALL_MEASURE,   // 3 seconden meten
  STATE_BLE_START,      // Start advertising
  STATE_BLE_WAIT        // Wacht op connectie of timeout
};

SystemState currentState = STATE_HALL_MEASURE;
unsigned long stateStartTime = 0;

const unsigned long HALL_MEASURE_TIME = 3000; // 3 seconden meten
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

  // ==========================================================
// LOGICA VOOR STILSTAND (Ultra Low Power)
// ==========================================================
if (Staatstil) {
    Serial.println("Slaapmodus actief. GPS & IMU uit.");
    Serial.flush(); 

    // --- 1. RANDAPPARATUUR UITZETTEN ---
    
    // GPS Uitschakelen (Belangrijkste besparing!)
    if (gnssOnline) {
        gnss.disablePower(); 
    }

    // TIP: Zet ook je IMU in low power mode, anders trekt die nog ~3mA
    myICM.lowPower(true); // Of myICM.sleep(true); afhankelijk van je specifieke ICM lib versie
    // Als lowPower niet werkt in jouw lib, kun je vaak sleep(true) gebruiken of sensoren disablen.

    // --- 2. WAKEUP BRONNEN INSTELLEN ---
    gpio_wakeup_enable((gpio_num_t)HALL_PIN, GPIO_INTR_LOW_LEVEL);
    esp_sleep_enable_gpio_wakeup();

    // 1 uur timer
    esp_sleep_enable_timer_wakeup(3600000000ULL);

    // --- 3. START LIGHT SLEEP ---
    esp_light_sleep_start();

    // ===============================================
    // HIER WORDT DE CODE HERVAT NA ONTWAKEN
    // ===============================================

    esp_sleep_wakeup_cause_t wakeup_reason = esp_sleep_get_wakeup_cause();

    // --- 4. RANDAPPARATUUR WEER AANZETTEN ---
    
    // GPS Aanzetten
    if (gnssOnline) {
        gnss.enablePower();
        // GPS heeft even tijd nodig om weer te locken, maar behoudt vaak zijn "hot start" data
        // als de batterijspanning niet volledig weg is geweest.
        gnss.setGnss(gnss.eGPS); 
    }

    // IMU Aanzetten
    myICM.lowPower(false); // Of myICM.sleep(false);
    myICM.begin(Wire, AD0_VAL); // Vaak veilig om even opnieuw te initieren bij twijfel

    // --- 5. STANDAARD AFHANDELING ---
    gpio_wakeup_disable((gpio_num_t)HALL_PIN);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_TIMER);
    
    if (wakeup_reason == ESP_SLEEP_WAKEUP_TIMER) {
        Serial.println("WAKKER! Door Timer (Check-in).");
        pulseCount = 0; 
    } else {
        Serial.println("WAKKER! Door Beweging (Hall).");
        pulseCount = 1; 
    }
    
    Staatstil = false;
    currentState = STATE_HALL_MEASURE; 
    stateStartTime = millis();
    delay(100); 

    return; 
}

  // ==========================================================
  // NORMALE STATE MACHINE (Als we bewegen)
  // ==========================================================
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

        if (currentKmh < 1.0) { 
            stilstandTeller++; 
        } else {
            stilstandTeller = 0; 
        }

        Serial.printf("Meting: %lu pulsen, %.1f km/h (Stilstand teller: %d/%d)\n", 
                       pulses, currentKmh, stilstandTeller, STILSTAND_LIMIET);

        currentState = STATE_BLE_START;

        if (stilstandTeller >= STILSTAND_LIMIET) {
            Serial.println("Detectie: 5x stilstand -> Activeer SLAAPMODUS");
            Staatstil = true;
            stilstandTeller = 0; 
        } else {
            Staatstil = false;
        }
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
        
        char latDirection = 'N'; char lonDirection = 'E';//In geval van geen GPS signaal
        long latIntDegree = 51060936; long lonIntDegree = 3708244;//In geval van geen GPS signaal
        
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