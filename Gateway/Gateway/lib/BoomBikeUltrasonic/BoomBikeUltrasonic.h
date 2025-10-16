// Ultrasonic sensor wrapper for BoomBike project
// Provides distance measurement and pass detection
// To enable pass detection, call enablePassDetection() and poll checkForPass() regularly
// by Kjell

#ifndef BOOMBIKEULTRASONIC_H
#define BOOMBIKEULTRASONIC_H

#include <Arduino.h>
#include <Ultrasonic.h>

class BoomBikeUltrasonic {
private:
    uint8_t _triggerPin;
    uint8_t _echoPin;
    Ultrasonic _sonar;
    bool _active = false;

    float _lastDistance = 0;
    unsigned long _lastReadTime = 0;
    unsigned long _lastTriggerTime = 0;
    
    // Tunable parameters for pass detection
    unsigned long _readInterval = 100; // Minimum time between reads in ms
    float _distanceThreshold = 30.0; // Minimum change in distance to consider a pass in cm
    unsigned long _cooldownTime = 1000; // Cooldown time after a detected pass in ms
public:
    BoomBikeUltrasonic(uint8_t triggerPin, uint8_t echoPin);

    float readDistance();
    bool checkForPass(); // returns true on a sudden change in distance

    void enablePassDetection() { _active = true; }
    void disablePassDetection() { _active = false; }
    bool isPassDetectionActive() const { return _active; }

    void setTimeResolution(unsigned long interval=100) { _readInterval = interval; }
    void setDistanceThreshold(float threshold=30.0) { _distanceThreshold = threshold; }
    void setCooldownTime(unsigned long cooldown=1000) { _cooldownTime = cooldown; }

};

#endif // BOOMBIKEULTRASONIC_H
