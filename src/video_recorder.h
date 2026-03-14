#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include "config.h"

// AVI/MJPEG structuur helpers
struct AviIndex {
    uint32_t offset;
    uint32_t size;
};

class VideoRecorder {
public:
    bool begin(const char* filename);
    bool writeFrame(const uint8_t* data, size_t len);
    bool finish();

    bool isRecording() const { return _recording; }
    const char* currentFile() const { return _filename; }
    uint32_t    frameCount()  const { return _frameCount; }

private:
    File     _file;
    char     _filename[64];
    bool     _recording   = false;
    uint32_t _frameCount  = 0;
    uint32_t _dataStart   = 0;    // byte offset waar movi chunk begint
    uint32_t _moviSize    = 0;

    // AVI index in RAM (max ~200 frames, past in SRAM)
    static const int MAX_INDEX = 1800; // 60s * 30fps marge
    AviIndex _index[MAX_INDEX];

    void writeAviHeader(uint32_t width, uint32_t height);
    void finalizeAviHeader();
    void writeIndex();

    uint32_t _width  = 0;
    uint32_t _height = 0;
    uint32_t _startMs = 0;
};
