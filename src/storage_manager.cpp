#include "storage_manager.h"
#include <time.h>

bool StorageManager::begin() {
    // SD_MMC gebruikt de ingebouwde SD-kaart interface van de AI Thinker ESP32-CAM
    // 1-bit modus voor compatibiliteit; hogere snelheid via 4-bit is ook mogelijk
    if (!SD_MMC.begin("/sdcard", true)) { // true = 1-bit modus
#if DEBUG_SERIAL
        Serial.println("[SD] Initialisatie mislukt");
#endif
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
#if DEBUG_SERIAL
        Serial.println("[SD] Geen SD-kaart gevonden");
#endif
        return false;
    }

#if DEBUG_SERIAL
    Serial.printf("[SD] Kaart type: %s\n",
        cardType == CARD_MMC  ? "MMC"  :
        cardType == CARD_SD   ? "SD"   :
        cardType == CARD_SDHC ? "SDHC" : "onbekend");
    Serial.printf("[SD] Capaciteit: %llu MB\n", SD_MMC.totalBytes() / (1024*1024));
#endif

    return ensureVideoDir();
}

bool StorageManager::ensureVideoDir() {
    if (!SD_MMC.exists(VIDEO_DIR)) {
        if (!SD_MMC.mkdir(VIDEO_DIR)) {
#if DEBUG_SERIAL
            Serial.printf("[SD] Map aanmaken mislukt: %s\n", VIDEO_DIR);
#endif
            return false;
        }
    }
    return true;
}

bool StorageManager::hasEnoughSpace() const {
    return freeBytes() > (uint64_t)MIN_FREE_MB * 1024 * 1024;
}

uint64_t StorageManager::totalBytes() const { return SD_MMC.totalBytes(); }
uint64_t StorageManager::usedBytes()  const { return SD_MMC.usedBytes(); }
uint64_t StorageManager::freeBytes()  const {
    return SD_MMC.totalBytes() - SD_MMC.usedBytes();
}

String StorageManager::freeMBStr() const {
    return String(freeBytes() / (1024*1024)) + " MB";
}

String StorageManager::totalMBStr() const {
    return String(totalBytes() / (1024*1024)) + " MB";
}

String StorageManager::newVideoFilename() const {
    struct tm ti;
    if (getLocalTime(&ti)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s/video_%04d-%02d-%02d_%02d-%02d-%02d.avi",
            VIDEO_DIR,
            ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
            ti.tm_hour, ti.tm_min, ti.tm_sec);
        return String(buf);
    }
    // Fallback: milliseconden als naam
    char buf[48];
    snprintf(buf, sizeof(buf), "%s/video_%lu.avi", VIDEO_DIR, millis());
    return String(buf);
}

std::vector<VideoFile> StorageManager::listVideos() {
    std::vector<VideoFile> videos;

    File dir = SD_MMC.open(VIDEO_DIR);
    if (!dir || !dir.isDirectory()) return videos;

    File f = dir.openNextFile();
    while (f) {
        if (!f.isDirectory()) {
            String fullPath = String(f.name());
            // f.name() geeft op ESP32 SD_MMC het volledige pad terug (/videos/naam.avi)
            // Haal alleen de bestandsnaam op (na de laatste '/')
            int slash = fullPath.lastIndexOf('/');
            String fname = (slash >= 0) ? fullPath.substring(slash + 1) : fullPath;

            if (fname.endsWith(".avi") || fname.endsWith(".AVI")) {
                VideoFile vf;
                vf.name    = fname;
                vf.size    = f.size();
                vf.sizeStr = String(f.size() / 1024) + " KB";
                videos.push_back(vf);
            }
        }
        f = dir.openNextFile();
    }
    dir.close();

    // Sorteer op naam (timestamp) – nieuwste eerst
    std::sort(videos.begin(), videos.end(), [](const VideoFile& a, const VideoFile& b){
        return a.name > b.name;
    });

    return videos;
}

bool StorageManager::deleteVideo(const char* filename) {
    // Bestandsnaam kan met of zonder pad binnenkomen
    String path = String(filename);
    if (!path.startsWith("/")) {
        path = String(VIDEO_DIR) + "/" + path;
    }
    if (SD_MMC.exists(path.c_str())) {
        return SD_MMC.remove(path.c_str());
    }
    return false;
}

bool StorageManager::videoExists(const char* filename) {
    String path = String(filename);
    if (!path.startsWith("/")) {
        path = String(VIDEO_DIR) + "/" + path;
    }
    return SD_MMC.exists(path.c_str());
}
