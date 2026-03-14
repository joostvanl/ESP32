#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include <vector>
#include "config.h"

struct VideoFile {
    String name;
    size_t size;
    String sizeStr;
};

class StorageManager {
public:
    bool begin();

    bool        hasEnoughSpace() const;
    uint64_t    totalBytes()     const;
    uint64_t    usedBytes()      const;
    uint64_t    freeBytes()      const;
    String      freeMBStr()      const;
    String      totalMBStr()     const;

    // Genereer unieke bestandsnaam op basis van tijd
    String      newVideoFilename() const;

    // Lijst van video's (gesorteerd, nieuwste eerst)
    std::vector<VideoFile> listVideos();

    bool deleteVideo(const char* filename);
    bool videoExists(const char* filename);

    // Zorg dat /videos map bestaat
    bool ensureVideoDir();
};
