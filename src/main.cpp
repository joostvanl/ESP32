#include <Arduino.h>
#include "config.h"
#include "wifi_manager.h"
#include "camera_control.h"
#include "motion_detection.h"
#include "video_recorder.h"
#include "storage_manager.h"
#include "web_server.h"

// ─── Globale objecten ─────────────────────────────────────────────────────────
WiFiManager      wifiMgr;
CameraControl    camera;
MotionDetection  motion;
VideoRecorder    recorder;
StorageManager   storage;
WebServerManager webServer(storage, camera);

// ─── Opname state ─────────────────────────────────────────────────────────────
enum class State { IDLE, RECORDING };
static State         appState      = State::IDLE;
static unsigned long recStartMs    = 0;
static unsigned long lastCleanupMs = 0;
static const unsigned long CLEANUP_INTERVAL_MS = 3600000UL; // 1 uur

// ─── Helpers ──────────────────────────────────────────────────────────────────

void startRecording() {
    if (appState == State::RECORDING) return;
    if (webServer.isStreaming) {
#if DEBUG_SERIAL
        Serial.println("[Main] Live stream actief, opname uitgesteld");
#endif
        return;
    }
    if (!storage.hasEnoughSpace()) {
#if DEBUG_SERIAL
        Serial.println("[Main] Onvoldoende opslagruimte, opname overgeslagen");
#endif
        return;
    }

    String filename = storage.newVideoFilename();
    if (!recorder.begin(filename.c_str())) return;

#if IR_LED_ENABLED
    camera.setIrLed(true);
#endif

    appState            = State::RECORDING;
    recStartMs          = millis();
    webServer.isRecording = true;

#if DEBUG_SERIAL
    Serial.printf("[Main] Opname gestart: %s\n", filename.c_str());
#endif
}

void stopRecording() {
    if (appState != State::RECORDING) return;

    recorder.finish();
    appState              = State::IDLE;
    webServer.isRecording = false;

#if IR_LED_ENABLED
    camera.setIrLed(false);
#endif

#if DEBUG_SERIAL
    Serial.println("[Main] Opname gestopt");
#endif
}

// ─── Setup ────────────────────────────────────────────────────────────────────

void setup() {
#if DEBUG_SERIAL
    Serial.begin(SERIAL_BAUD);
    delay(500);
    Serial.println("\n=== NestboxCam opstarten ===");
#endif

    // SD-kaart EERST initialiseren (vóór camera)
    // AI Thinker ESP32-CAM: GPIO 4 = flash LED én SD_MMC HS2_DATA1
    // Camera-init claimt GPIO 4 als output; SD moet dus voor camera starten
    if (!storage.begin()) {
#if DEBUG_SERIAL
        Serial.println("[Main] FOUT: SD-kaart niet beschikbaar – herstart over 10s");
#endif
        delay(10000);
        ESP.restart();
    }

    // Camera initialiseren NA SD
    if (!camera.begin()) {
#if DEBUG_SERIAL
        Serial.println("[Main] FOUT: Camera niet beschikbaar – systeem loopt door");
#endif
    }

    // WiFi
    wifiMgr.begin();

    // PIR
    motion.begin();

    // Webserver starten zodra WiFi verbonden is (of direct, zodat AP werkt)
    webServer.begin();

#if DEBUG_SERIAL
    Serial.printf("[Main] Setup klaar. IP: %s\n",
        wifiMgr.isConnected() ? wifiMgr.localIP().c_str() : "geen WiFi");
#endif
}

// ─── Loop ─────────────────────────────────────────────────────────────────────

void loop() {
    // WiFi bewaken
    wifiMgr.loop();

    // Webserver verwerken
    webServer.loop();

    // PIR warmup bewaken
    motion.loop();

    switch (appState) {
        case State::IDLE: {
            // Controleer op beweging
            if (motion.motionDetected()) {
#if DEBUG_SERIAL
                Serial.println("[Main] Beweging gedetecteerd!");
#endif
                startRecording();
            }
            break;
        }

        case State::RECORDING: {
            unsigned long now = millis();
            uint32_t elapsed  = (uint32_t)(now - recStartMs) / 1000;

            // Opname tijdslimiet
            if (now - recStartMs >= (unsigned long)RECORD_DURATION_SEC * 1000) {
                Serial.printf("[Main] Opnameduur bereikt (%u sec), stoppen\n", elapsed);
                stopRecording();
                break;
            }

            // Live stream voorkomt opname
            if (webServer.isStreaming) {
                Serial.println("[Main] Stream actief, opname gestopt");
                stopRecording();
                break;
            }

            const unsigned long frameInterval = 1000 / CAM_FPS_TARGET;
            static unsigned long lastFrameMs = 0;
            if (lastFrameMs < recStartMs) lastFrameMs = recStartMs;

            if (now - lastFrameMs >= frameInterval) {
                lastFrameMs = now;

                camera_fb_t* fb = camera.captureFrame();
                if (!fb) {
                    Serial.println("[Main] WAARSCHUWING: camera.captureFrame() gaf nullptr");
                } else {
                    if (fb->len == 0) {
                        Serial.println("[Main] WAARSCHUWING: frame len=0");
                    } else if (!recorder.writeFrame(fb->buf, fb->len)) {
                        Serial.println("[Main] writeFrame mislukt – opname stopt");
                        camera.releaseFrame(fb);
                        stopRecording();
                        break;
                    }
                    camera.releaseFrame(fb);
                }
            }

            // SD ruimte bewaken
            if (!storage.hasEnoughSpace()) {
                Serial.println("[Main] SD bijna vol – opname stopt");
                stopRecording();
            }
            break;
        }
    }

    // Auto-cleanup: verwijder video's ouder dan 2 dagen (elk uur, alleen in IDLE)
    if (appState == State::IDLE &&
        (lastCleanupMs == 0 || millis() - lastCleanupMs >= CLEANUP_INTERVAL_MS)) {
        lastCleanupMs = millis();
        storage.deleteOldVideos(2);
    }

    // Kleine delay om watchdog te voeden en CPU niet te verzadigen
    delay(1);
}
