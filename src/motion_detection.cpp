#include "motion_detection.h"

MotionDetection* MotionDetection::_instance = nullptr;

void IRAM_ATTR MotionDetection::pirISR() {
    if (_instance) _instance->_triggered = true;
}

void MotionDetection::begin() {
    _instance  = this;
    _startTime = millis();
    _active    = false;
    _triggered = false;
    _warmupDone = false;

    pinMode(PIR_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(PIR_PIN), pirISR, RISING);

#if DEBUG_SERIAL
    Serial.printf("[PIR] Opwarmen gedurende %d seconden...\n", PIR_WARMUP_MS / 1000);
#endif
}

void MotionDetection::loop() {
    if (!_warmupDone) {
        if (millis() - _startTime >= PIR_WARMUP_MS) {
            _warmupDone = true;
            _triggered  = false; // valse triggers tijdens opwarmen wissen
#if DEBUG_SERIAL
            Serial.println("[PIR] Klaar, bewegingsdetectie actief");
#endif
        }
    }
}

bool MotionDetection::motionDetected() {
    if (!_warmupDone) return false;
    unsigned long now = millis();
    if (now - _lastTriggerMs < (unsigned long)PIR_DEBOUNCE_MS)
        return false;
    if (_triggered) {
        _triggered = false;
        _lastTriggerMs = now;
        _active = true;
        return true;
    }
    return false;
}
