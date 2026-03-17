# NestboxCam

Compact nestkast camerasysteem op basis van de **ESP32-CAM (AI Thinker)** met:
- Automatische **bewegingsgestuurde video-opname** (PIR sensor)
- **AVI/MJPEG** opslag op SD-kaart
- **Live MJPEG stream** via browser
- **Webinterface** voor beheer van video's en opslagruimte

---

## Hardware

| Component | Specificatie |
|---|---|
| Microcontroller | ESP32-CAM (AI Thinker) |
| Camera | OV2640 (ingebouwd) |
| Bewegingssensor | HC-SR501 of mini PIR |
| SD-kaart | MicroSD, FAT32, max 32 GB |
| Voeding | 5V via USB of externe voeding |
| IR verlichting | Optioneel via GPIO 4 (flash LED) |

---

## Bedrading

```
ESP32-CAM          PIR Sensor (HC-SR501)
──────────         ──────────────────────
GPIO 13      ──→   OUT
GND          ──→   GND
3.3V of 5V   ──→   VCC  (HC-SR501 werkt op 5V)

ESP32-CAM          IR LED (optioneel)
──────────         ──────────────────
GPIO 4       ──→   Anode (+) via 33Ω weerstand
GND          ──→   Kathode (-)
```

> **Let op:** De ingebouwde flash LED zit op GPIO 4. Gebruik `IR_LED_ENABLED true` in `config.h` om deze in te schakelen tijdens opname.

---

## Installatie

### Via PlatformIO (aanbevolen)

```bash
# Open de NestboxCam map in VS Code met PlatformIO extensie
# Pas config.h aan (WiFi credentials, pinansluitingen)
# Selecteer het board: ESP32-CAM
pio run --target upload
```

### Via Arduino IDE

1. Installeer **ESP32 board support** via Boards Manager
2. Selecteer board: `AI Thinker ESP32-CAM`
3. Kopieer alle `.h` en `.cpp` bestanden uit `src/` naar één map
4. Hernoem `main.cpp` naar `NestboxCam.ino`
5. Stel in: Tools → Partition Scheme → **Huge APP**
6. Upload met 921600 baud

---

## Configuratie (`src/config.h`)

Pas minimaal de WiFi-instellingen aan vóór het uploaden:

```cpp
#define WIFI_SSID        "JouwSSID"
#define WIFI_PASSWORD    "JouwWachtwoord"
#define USE_HTTPS        1      // 1 = HTTPS (poort 443, self-signed cert), 0 = HTTP (poort 80)
#define PIR_PIN          13      // GPIO van PIR sensor
#define PIR_DEBOUNCE_MS  5000    // na een trigger geen nieuwe opname (anti-dubbel)
#define RECORD_DURATION_SEC  60  // seconden per opname
#define IR_LED_ENABLED   false   // true = IR LED aan tijdens opname
#define MIN_FREE_MB      50      // stop opname bij < 50 MB vrij
```

### Resolutie instellen

Wanneer je de resolutie wijzigt, moet je **beide** onderstaande defines tegelijk aanpassen:

```cpp
#define CAM_FRAME_SIZE   FRAMESIZE_VGA  // enum voor camera driver
#define CAM_FRAME_WIDTH  640            // breedte in pixels
#define CAM_FRAME_HEIGHT 480            // hoogte in pixels
```

> **Belangrijk:** `CAM_FRAME_WIDTH` en `CAM_FRAME_HEIGHT` worden gebruikt voor de AVI-header. Als ze niet overeenkomen met `CAM_FRAME_SIZE` wordt de video verkeerd opgeslagen.

Beschikbare resoluties:

| `CAM_FRAME_SIZE` | `CAM_FRAME_WIDTH` | `CAM_FRAME_HEIGHT` | Opmerking |
|---|---|---|---|
| `FRAMESIZE_QQVGA` | 160 | 120 | Minimaal RAM |
| `FRAMESIZE_QVGA` | 320 | 240 | Weinig RAM |
| `FRAMESIZE_CIF` | 400 | 296 | Fallback zonder PSRAM |
| `FRAMESIZE_VGA` | 640 | 480 | **Standaard (aanbevolen)** |
| `FRAMESIZE_SVGA` | 800 | 600 | Vereist PSRAM |
| `FRAMESIZE_XGA` | 1024 | 768 | Vereist PSRAM |

---

## Automatische opruiming

Video's ouder dan **2 dagen** worden automatisch verwijderd. Dit gebeurt:

- Direct bij het opstarten (zodra NTP gesynchroniseerd is)
- Daarna elk uur, zolang het systeem in de **wachtstand** staat (geen actieve opname)

De leeftijd wordt bepaald op basis van de datum in de bestandsnaam (`video_YYYY-MM-DD_HH-MM-SS.avi`). Als NTP nog niet gesynchroniseerd is, wordt de cleanup overgeslagen.

In de seriële log is dit zichtbaar als:

```
[SD] Auto-cleanup: verwijder video_2026-03-12_10-30-00.avi (2.5 dagen oud)
[SD] Auto-cleanup: 1 video('s) verwijderd (ouder dan 2 dag(en))
```

---

## HTTPS (optioneel)

In `config.h` kun je **`USE_HTTPS`** op `1` zetten. De server draait dan op **poort 443** met een **self-signed certificaat** (gegenereerd bij elke start, eerste keer ca. 30 seconden). Geen beveiligingswaarschuwing meer bij download of afspelen; de browser vraagt één keer om het certificaat te vertrouwen (bijv. “Geavanceerd” → “Doorgaan naar …”).

- **`USE_HTTPS 1`**: alleen HTTPS op poort 443 (aanbevolen).
- **`USE_HTTPS 0`**: HTTP op poort 80, LED-API op poort 81 (oorspronkelijk gedrag).

### Fout: `hwcrypto/sha.h: No such file or directory`

Bij nieuwere versies van de ESP32 Arduino core is dit headerbestand verplaatst. **PlatformIO:** een pre-build script past de library automatisch aan; gewoon opnieuw bouwen (`pio run`) is voldoende. **Arduino IDE:** open in je Arduino-libraries map het bestand `esp32_https_server/src/HTTPConnection.hpp` (of de .cpp waarin de include staat) en vervang `#include <hwcrypto/sha.h>` door `#include <esp32/sha.h>`.

---

## Browser: download en afspelen

Video's worden **via een blob** geladen (fetch + object URL). Daardoor:
- Geen beveiligingsmelding bij download (bij HTTP komt de download uit een blob; bij HTTPS is de verbinding al beveiligd).
- Afspelen in de browser: de player laadt de video via fetch en speelt de blob af. Ondersteunt je browser AVI/MJPEG niet, gebruik dan de downloadknop en open met VLC.

De server ondersteunt **Range-requests** (`Accept-Ranges: bytes`, 206 Partial Content) voor videostreaming.

De LED-knop op de live-pagina: bij **HTTP** via poort **81** (zodat die tijdens de stream werkt); bij **HTTPS** via dezelfde verbinding (`/api/led`).

---

## Seriële logging (debuggen)

Het systeem stuurt gedetailleerde logberichten via **Serial (UART)** op **115200 baud**.

### Logs inzien

**Arduino IDE**
1. Sluit de ESP32-CAM aan via USB
2. Tools → **Serial Monitor** (`Ctrl+Shift+M`)
3. Stel baudrate rechtsonder in op **`115200`**
4. Druk op de reset-knop om vanaf het begin te loggen

**PlatformIO**
```bash
pio device monitor --baud 115200
```

**PuTTY / CoolTerm / andere terminal**
- Verbinding: Serial, juiste COM-poort, 115200 baud

### Wat je te zien krijgt

Opstarten:
```
=== NestboxCam opstarten ===
[SD] Kaart type: SDHC
[SD] Capaciteit: 252 MB, Gebruikt: 49 MB, Vrij: 203 MB
[SD] Schrijftest OK
[Camera] Geïnitialiseerd
[Camera] PSRAM beschikbaar
[WiFi] Verbonden! IP: 192.168.1.42
[Web] Server gestart op poort 80
[PIR] Opwarmen gedurende 30 seconden...
```

Tijdens een opname:
```
[Main] Beweging gedetecteerd!
[REC] begin() → /videos/video_2026-03-15_23-16-08.avi
[REC] Header geschreven: 248/248 bytes, positie=248
[REC] Frame #1: 8432 bytes, movi=8440, filepos=8688
[REC] Frame #10: 8201 bytes, ...
[REC] finish() – 600 frames, moviSize=4892016
[REC] Bestandsgrootte na close: 5101648 bytes
[REC] Header OK (RIFF gevonden)
[REC] Opname voltooid: 600 frames in 60 sec
```

### Aandachtspunten bij problemen

| Wat zoeken in de log | Betekenis |
|---|---|
| `FOUT:` | Kritieke fout |
| `WAARSCHUWING:` | Iets klopt niet maar gaat door |
| `Schrijftest OK` / `MISLUKT` | Is de SD-kaart schrijfbaar? |
| `Header geschreven: X/248` | Moet exact 248 zijn |
| `Frame #1:` aanwezig? | Worden er überhaupt frames opgenomen? |
| `Bestandsgrootte na close: X` | Grootte vóór header-patch |
| `Finale bestandsgrootte: X` | 0 = patch wiste het bestand! |
| `Eerste 4 bytes: RIFF` | Header correct geschreven |

---

## Webinterface

Na opstarten zie je het IP-adres in de seriële monitor:

```
[WiFi] Verbonden! IP: 192.168.1.42
[Web] Server gestart op poort 80
```

Open `http://192.168.1.42` in je browser.

| Pad | Beschrijving |
|---|---|
| `/` | Dashboard – status, opslag, acties |
| `/live` | Live MJPEG stream |
| `/videos` | Lijst van opgeslagen video's |
| `/play?file=naam.avi` | Video afspelen |
| `/download?file=naam.avi` | Video downloaden |
| `/delete?file=naam.avi` | Video verwijderen |
| `/api/status` | JSON systeemstatus |
| `/api/videos` | JSON videolijst |

---

## SD-kaart structuur

```
/
└── videos/
    ├── video_2026-03-14_18-21-33.avi
    ├── video_2026-03-14_20-02-10.avi
    └── ...
```

Bestandsformaat: **AVI container met MJPEG frames**  
Afspelen: elke moderne mediaspeler (VLC, Windows Media Player, browser).

---

## Architectuur

```
main.cpp
 ├── wifi_manager      – WiFi verbinding + NTP tijdsync
 ├── camera_control    – OV2640 initialisatie + frame capture
 ├── motion_detection  – PIR interrupt + opwarmtijd
 ├── video_recorder    – AVI/MJPEG schrijven naar SD
 ├── storage_manager   – SD-kaart, bestandsbeheer
 └── web_server        – HTTP routes + MJPEG live stream
```

### State machine

```
IDLE ──[PIR trigger]──→ RECORDING
         ↑                  │
         └──[60s / SD vol]──┘
```

---

## Bekende beperkingen

- **Live stream + opname tegelijk** is niet ondersteund vanwege RAM-beperkingen. De webserver toont een melding als je tijdens een opname naar `/live` gaat.
- **PSRAM** (aanwezig op AI Thinker ESP32-CAM) wordt automatisch gebruikt voor hogere resolutie en meerdere frame buffers. Zonder PSRAM valt de camera terug op `FRAMESIZE_CIF`.
- **PIR opwarmtijd**: De HC-SR501 heeft ~30 seconden nodig na inschakelen. Valse triggers worden in deze periode genegeerd.
- **SD-kaart**: Gebruik een FAT32 geformatteerde kaart van maximaal 32 GB voor beste compatibiliteit.
- **Partitie schema**: Gebruik `Huge APP` (geen OTA) om voldoende flash ruimte te hebben voor de firmware.
- **Resolutie wijzigen**: Pas altijd zowel `CAM_FRAME_SIZE` als `CAM_FRAME_WIDTH`/`CAM_FRAME_HEIGHT` aan – anders bevat de AVI-header verkeerde dimensies.

---

## Mogelijke uitbreidingen

- [ ] Webconfiguratie voor WiFi (AP mode)
- [ ] Automatische cleanup bij volle SD
- [ ] PIR gevoeligheid instelling via webinterface
- [ ] Snapshot modus (foto in plaats van video)
- [ ] Nachtmodus met IR LED planning
- [ ] Pushmelding via Telegram Bot API
