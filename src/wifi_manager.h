#pragma once
#include <WiFi.h>
#include "config.h"

class WiFiManager {
public:
    void begin();
    void loop();
    bool isConnected() const { return WiFi.status() == WL_CONNECTED; }
    String localIP() const   { return WiFi.localIP().toString(); }

private:
    unsigned long _lastAttempt = 0;
    bool          _timesynced  = false;

    void connect();
    void syncTime();
};
