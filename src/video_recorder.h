#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include "config.h"

class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    bool begin(const char* filename);
    bool writeFrame(const uint8_t* data, size_t len);
    bool finish();

    bool        isRecording() const { return _recording; }
    const char* currentFile() const { return _filename; }
    uint32_t    frameCount()  const { return _frameCount; }

private:
    File     _file;
    char     _filename[64];
    bool     _recording  = false;
    uint32_t _frameCount = 0;
    uint32_t _moviSize   = 0;
    uint32_t _startMs    = 0;

    // AVI index op de heap om stack-overflow te voorkomen
    // Elke entry: 4 bytes offset + 4 bytes size = 8 bytes
    // 600 entries = max 60s @ 10fps
    static const uint32_t MAX_INDEX = 600;
    struct AviIdx { uint32_t offset; uint32_t size; };
    AviIdx* _index = nullptr;

    // Hulpfuncties AVI schrijven
    void writeU32LE(uint32_t v);
    void writeU16LE(uint16_t v);
    void writeFourCC(const char* cc);
    void writeAviHeader(uint32_t width, uint32_t height);
    void patchU32(uint32_t fileOffset, uint32_t value);
};
