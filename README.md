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
#define PIR_PIN          13      // GPIO van PIR sensor
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
