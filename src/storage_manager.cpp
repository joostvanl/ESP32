#include "storage_manager.h"
#include <time.h>

bool StorageManager::begin() {
    if (!SD_MMC.begin("/sdcard", true)) {
        Serial.println("[SD] Initialisatie mislukt");
        return false;
    }

    uint8_t cardType = SD_MMC.cardType();
    if (cardType == CARD_NONE) {
        Serial.println("[SD] Geen SD-kaart gevonden");
        return false;
    }

    Serial.printf("[SD] Kaart type: %s\n",
        cardType == CARD_MMC  ? "MMC"  :
        cardType == CARD_SD   ? "SD"   :
        cardType == CARD_SDHC ? "SDHC" : "onbekend");
    Serial.printf("[SD] Capaciteit: %llu MB, Gebruikt: %llu MB, Vrij: %llu MB\n",
        SD_MMC.totalBytes()/(1024*1024),
        SD_MMC.usedBytes()/(1024*1024),
        (SD_MMC.totalBytes()-SD_MMC.usedBytes())/(1024*1024));

    if (!ensureVideoDir()) return false;

    // ── SD schrijftest ────────────────────────────────────────────────────
    Serial.println("[SD] Schrijftest uitvoeren...");
    const char* testPath = "/videos/_sdtest.tmp";
    File t = SD_MMC.open(testPath, FILE_WRITE);
    if (!t) {
        Serial.println("[SD] FOUT: Schrijftest bestand aanmaken mislukt!");
        return false;
    }
    const char* testData = "NestboxCam SD test 1234567890";
    size_t wr = t.print(testData);
    t.close();
    Serial.printf("[SD] Schrijftest: %u bytes geschreven\n", wr);

    // Verifieer
    File tr = SD_MMC.open(testPath, FILE_READ);
    if (tr) {
        Serial.printf("[SD] Schrijftest bestand grootte: %u bytes\n", (uint32_t)tr.size());
        tr.close();
        SD_MMC.remove(testPath);
        Serial.println("[SD] Schrijftest OK");
    } else {
        Serial.println("[SD] FOUT: Schrijftest bestand niet terug te lezen!");
        return false;
    }

    return true;
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

String StorageManager::newPhotoFilename() const {
    struct tm ti;
    if (getLocalTime(&ti)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%s/photo_%04d-%02d-%02d_%02d-%02d-%02d.jpg",
            VIDEO_DIR,
            ti.tm_year + 1900, ti.tm_mon + 1, ti.tm_mday,
            ti.tm_hour, ti.tm_min, ti.tm_sec);
        return String(buf);
    }
    char buf[48];
    snprintf(buf, sizeof(buf), "%s/photo_%lu.jpg", VIDEO_DIR, millis());
    return String(buf);
}

static bool isSafeMediaName(const String& name) {
    if (name.length() == 0 || name.length() > 120) return false;
    if (name.indexOf("..") >= 0 || name.indexOf('/') >= 0 || name.indexOf('\\') >= 0)
        return false;
    return name.endsWith(".avi") || name.endsWith(".AVI") ||
           name.endsWith(".jpg") || name.endsWith(".JPG") ||
           name.endsWith(".jpeg") || name.endsWith(".JPEG");
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

            if (fname.endsWith(".avi") || fname.endsWith(".AVI") ||
                fname.endsWith(".jpg") || fname.endsWith(".JPG") ||
                fname.endsWith(".jpeg") || fname.endsWith(".JPEG")) {
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

    // Sorteer op tijdstempel in naam (video_/photo_ + YYYY-MM-DD_HH-MM-SS), nieuwste eerst.
    // Alleen lexicografisch op volledige naam is fout: 'v' > 'p' dus elke video ging vóór elke foto.
    auto sortKey = [](const String& name, String& keyOut) -> bool {
        if (!(name.startsWith("video_") || name.startsWith("photo_"))) return false;
        if (name.length() < 6 + 19) return false;
        keyOut = name.substring(6, 25);
        return true;
    };

    std::sort(videos.begin(), videos.end(), [&sortKey](const VideoFile& a, const VideoFile& b) -> bool {
        String ka, kb;
        bool okA = sortKey(a.name, ka);
        bool okB = sortKey(b.name, kb);
        if (okA && okB) {
            if (ka != kb) return static_cast<bool>(ka > kb);
            return static_cast<bool>(a.name > b.name);
        }
        if (okA != okB) return okA;
        return static_cast<bool>(a.name > b.name);
    });

    return videos;
}

bool StorageManager::deleteVideo(const char* filename) {
    String path = String(filename);
    if (!path.startsWith("/")) {
        if (!isSafeMediaName(path)) return false;
        path = String(VIDEO_DIR) + "/" + path;
    } else {
        if (!path.startsWith(String(VIDEO_DIR) + "/")) return false;
    }
    if (SD_MMC.exists(path.c_str())) {
        return SD_MMC.remove(path.c_str());
    }
    return false;
}

bool StorageManager::videoExists(const char* filename) {
    String path = String(filename);
    if (!path.startsWith("/")) {
        if (!isSafeMediaName(path)) return false;
        path = String(VIDEO_DIR) + "/" + path;
    } else if (!path.startsWith(String(VIDEO_DIR) + "/")) {
        return false;
    }
    return SD_MMC.exists(path.c_str());
}

int StorageManager::deleteOldVideos(int maxAgeDays) {
    time_t now;
    time(&now);

    // Wacht tot NTP gesynchroniseerd is (tijd > 1 jan 2020)
    if (now < 1577836800L) {
        Serial.println("[SD] Auto-cleanup: NTP nog niet gesynchroniseerd, overgeslagen");
        return 0;
    }

    auto videos = listVideos();
    int deleted = 0;

    for (auto& v : videos) {
        // Bestandsnaam formaat: video_YYYY-MM-DD_HH-MM-SS.avi
        // Positie:              0123456789...
        if (v.name.length() < 22) continue;
        if (!v.name.startsWith("video_")) continue;

        int yr = v.name.substring(6, 10).toInt();
        int mo = v.name.substring(11, 13).toInt();
        int dy = v.name.substring(14, 16).toInt();

        if (yr < 2020 || mo < 1 || mo > 12 || dy < 1 || dy > 31) continue;

        struct tm t = {};
        t.tm_year  = yr - 1900;
        t.tm_mon   = mo - 1;
        t.tm_mday  = dy;
        t.tm_isdst = -1;
        time_t fileTime = mktime(&t);

        if (fileTime == (time_t)-1) continue;

        double ageSec = difftime(now, fileTime);
        if (ageSec > (double)maxAgeDays * 86400.0) {
            Serial.printf("[SD] Auto-cleanup: verwijder %s (%.1f dagen oud)\n",
                          v.name.c_str(), ageSec / 86400.0);
            if (deleteVideo(v.name.c_str())) deleted++;
        }
    }

    if (deleted > 0) {
        Serial.printf("[SD] Auto-cleanup: %d video('s) verwijderd (ouder dan %d dag(en))\n",
                      deleted, maxAgeDays);
    } else {
        Serial.println("[SD] Auto-cleanup: niets te verwijderen");
    }
    return deleted;
}
