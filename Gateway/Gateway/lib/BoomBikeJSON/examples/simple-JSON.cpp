// Simple example of using BoomBikeJSON library
// The JSON object is created, populated with some sample data, converted to a string, and printed to Serial.

#include "BoomBikeJSON.h"

// Data handling object
BoomBikeJSON bikeJSON;

void setup() {
  Serial.begin(115200);
  Serial.println("Starting BoomBike JSON example...");

  // Add some sample data
  bikeJSON.addData("speed", 24);
  bikeJSON.addData("temperature", 18.5);
  bikeJSON.addData("battery", 75);
  
  // Convert to JSON string and print
  const char* jsonString = bikeJSON.toString();
  Serial.println("Generated JSON:");
  Serial.println(jsonString);

  // Clear the JSON document for future use
  bikeJSON.clear();
}

void loop() {
  // Nothing to do here
}