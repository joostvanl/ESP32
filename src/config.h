#pragma once

// ─── WiFi ────────────────────────────────────────────────────────────────────
#define WIFI_SSID        "JouwSSID"
#define WIFI_PASSWORD    "JouwWachtwoord"
#define WIFI_RETRY_DELAY 5000   // ms tussen reconnect pogingen
#define WIFI_MAX_RETRIES 20     // pogingen voordat systeem herstart

// ─── Webserver ────────────────────────────────────────────────────────────────
#define WEB_PORT         80
#define WEB_PORT_HTTPS   443  // bij USE_HTTPS
#define LED_API_PORT     81   // alleen bij HTTP: aparte poort voor LED-API tijdens stream
#define USE_HTTPS        0    // 1 = HTTPS (vereist library esp32_https_server), 0 = alleen HTTP

// ─── Camera (AI Thinker ESP32-CAM) ───────────────────────────────────────────
#define CAM_PIN_PWDN     32
#define CAM_PIN_RESET    -1
#define CAM_PIN_XCLK      0
#define CAM_PIN_SIOD     26
#define CAM_PIN_SIOC     27
#define CAM_PIN_D7       35
#define CAM_PIN_D6       34
#define CAM_PIN_D5       39
#define CAM_PIN_D4       36
#define CAM_PIN_D3       21
#define CAM_PIN_D2       19
#define CAM_PIN_D1       18
#define CAM_PIN_D0        5
#define CAM_PIN_VSYNC    25
#define CAM_PIN_HREF     23
#define CAM_PIN_PCLK     22

// Resolutie: FRAMESIZE_SVGA = 800x600, FRAMESIZE_VGA = 640x480
// Kies één van: FRAMESIZE_QQVGA, FRAMESIZE_QVGA, FRAMESIZE_CIF,
//               FRAMESIZE_VGA, FRAMESIZE_SVGA, FRAMESIZE_XGA
#define CAM_FRAME_SIZE   FRAMESIZE_VGA
#define CAM_QUALITY      12     // 0-63, lager = betere kwaliteit, meer RAM
#define CAM_FPS_TARGET   10     // doelframe rate

// Bijbehorende pixelafmetingen (moet overeenkomen met CAM_FRAME_SIZE)
// Pas aan als je CAM_FRAME_SIZE wijzigt:
#define CAM_FRAME_WIDTH  640
#define CAM_FRAME_HEIGHT 480

// ─── PIR sensor ──────────────────────────────────────────────────────────────
#define PIR_PIN          13     // GPIO pin voor PIR sensor
#define PIR_WARMUP_MS    30000  // PIR opwarmtijd bij start (ms)
#define PIR_DEBOUNCE_MS  5000   // na een trigger X ms geen nieuwe opname (anti-dubbel)

// ─── IR LED ──────────────────────────────────────────────────────────────────
#define IR_LED_PIN        4     // ingebouwde flash LED, ook bruikbaar voor IR
#define IR_LED_ENABLED    false // zet op true om IR LED te activeren tijdens opname

// ─── Video opname ─────────────────────────────────────────────────────────────
#define RECORD_DURATION_SEC  60     // seconden per opname
#define VIDEO_DIR            "/videos"
#define AVI_HEADER_SIZE      240    // bytes voor AVI RIFF header

// ─── Opslagbeheer ─────────────────────────────────────────────────────────────
#define MIN_FREE_MB      50     // minimale vrije ruimte (MB) – stop opname als minder
#define MAX_VIDEOS       200    // maximaal aantal video's op SD

// ─── NTP tijd ─────────────────────────────────────────────────────────────────
#define NTP_SERVER       "pool.ntp.org"
#define NTP_TZ_OFFSET    3600   // UTC+1 (CET), gebruik 7200 voor CEST
#define NTP_DST_OFFSET   3600   // zomertijd offset

// ─── Debug ────────────────────────────────────────────────────────────────────
#define DEBUG_SERIAL     true
#define SERIAL_BAUD      115200
