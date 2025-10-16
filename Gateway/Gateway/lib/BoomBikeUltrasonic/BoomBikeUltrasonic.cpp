#include "BoomBikeUltrasonic.h"

BoomBikeUltrasonic::BoomBikeUltrasonic(uint8_t triggerPin, uint8_t echoPin)
    : _triggerPin(triggerPin), _echoPin(echoPin), _sonar(triggerPin, echoPin) {}

float BoomBikeUltrasonic::readDistance() {
    unsigned long distance = _sonar.read();
    return distance;
}

bool BoomBikeUltrasonic::checkForPass() {
    if (!_active) return false; // sensor not active

    unsigned long currentTime = millis();
    if (currentTime - _lastReadTime < _readInterval) {
        return false; // only read at defined intervals
    }

    _lastReadTime = currentTime;
    float currentDistance = readDistance();
    
    // Ignore invalid readings
    if (currentDistance <= 0 || currentDistance >= 400) {
        return false;
    }

    float delta = fabs(currentDistance - _lastDistance);
    _lastDistance = currentDistance;

    if (delta > _distanceThreshold && (currentTime - _lastTriggerTime) > _cooldownTime) {
        _lastTriggerTime = currentTime;
        return true; // pass detected and cooldown reset satisfied
    }
    
    return false;
}