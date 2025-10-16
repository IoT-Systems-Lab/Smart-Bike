// Print distance readings from the BoomBikeUltrasonic sensor every second

#include "BoomBikeUltrasonic.h"

// Ultrasonic sensor on pins 8 (trigger) and 9 (echo)
BoomBikeUltrasonic ultrasonic(8, 9);

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike Ultrasonic example...");
}

void loop() {
  // Read distance from the ultrasonic sensor
  float distance = ultrasonic.readDistance();
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");
  delay(1000);
}