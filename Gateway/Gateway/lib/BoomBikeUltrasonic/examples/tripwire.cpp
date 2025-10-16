// Example code for BoomBikeUltrasonic library
// Monitors an ultrasonic sensor for sudden changes in distance (a "pass")

#include "BoomBikeUltrasonic.h"

// Ultrasonic sensor on pins 8 (trigger) and 9 (echo)
BoomBikeUltrasonic ultrasonic(8, 9);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike Ultrasonic Test...");

  // Set parameters for pass detection
  ultrasonic.setTimeResolution(50); // read every 50 ms
  ultrasonic.setDistanceThreshold(20.0); // consider a pass if distance changes by 20 cm
  ultrasonic.setCooldownTime(2000); // 2 seconds cooldown between passes

  ultrasonic.enablePassDetection();
}

void loop() {
    if (ultrasonic.isPassDetectionActive()) { // only check if pass detection is enabled
        // Poll for pass events, returns true on a pass
        if (ultrasonic.checkForPass()) {
            Serial.println("Pass detected!");
        }
    }
}