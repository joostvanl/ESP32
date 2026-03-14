#pragma once
#include <Arduino.h>
#include "config.h"

class MotionDetection {
public:
    void begin();
    void loop();

    bool motionDetected();   // geeft true als er nieuwe beweging is (éénmalig)
    bool isActive() const { return _active; }

private:
    volatile bool _triggered = false;
    bool          _active    = false;
    bool          _warmupDone = false;
    unsigned long _startTime  = 0;

    static void IRAM_ATTR pirISR();
    static MotionDetection* _instance;
};
