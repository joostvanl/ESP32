#include "wifi_manager.h"
#include <time.h>

void WiFiManager::begin() {
    WiFi.mode(WIFI_STA);
    WiFi.setAutoReconnect(true);
    connect();
}

void WiFiManager::loop() {
    if (!isConnected()) {
        unsigned long now = millis();
        if (now - _lastAttempt >= WIFI_RETRY_DELAY) {
            _lastAttempt = now;
            connect();
        }
        return;
    }

    if (!_timesynced) {
        syncTime();
    }
}

void WiFiManager::connect() {
#if DEBUG_SERIAL
    Serial.printf("[WiFi] Verbinden met %s...\n", WIFI_SSID);
#endif
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int tries = 0;
    while (WiFi.status() != WL_CONNECTED && tries < WIFI_MAX_RETRIES) {
        delay(500);
        tries++;
    }

    if (isConnected()) {
#if DEBUG_SERIAL
        Serial.printf("[WiFi] Verbonden! IP: %s\n", WiFi.localIP().toString().c_str());
#endif
        _timesynced = false; // forceer NTP sync
    } else {
#if DEBUG_SERIAL
        Serial.println("[WiFi] Verbinding mislukt, wordt opnieuw geprobeerd...");
#endif
    }
}

void WiFiManager::syncTime() {
#if DEBUG_SERIAL
    Serial.println("[WiFi] NTP tijd synchroniseren...");
#endif
    configTime(NTP_TZ_OFFSET, NTP_DST_OFFSET, NTP_SERVER);

    struct tm ti;
    int retries = 0;
    while (!getLocalTime(&ti) && retries < 10) {
        delay(500);
        retries++;
    }

    if (retries < 10) {
        _timesynced = true;
#if DEBUG_SERIAL
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &ti);
        Serial.printf("[WiFi] Tijd gesynchroniseerd: %s\n", buf);
#endif
    } else {
#if DEBUG_SERIAL
        Serial.println("[WiFi] NTP sync mislukt");
#endif
    }
}
