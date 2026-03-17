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

    static const uint32_t HEADER_SIZE = 248;

private:
    File     _file;
    char     _filename[64];
    char     _vfsPath[80];   // /sdcard + _filename voor fopen
    bool     _recording  = false;
    uint32_t _frameCount = 0;
    uint32_t _moviSize   = 0;
    uint32_t _startMs    = 0;

    static const uint32_t MAX_INDEX = 600;
    struct AviIdx { uint32_t offset; uint32_t size; };
    AviIdx* _index = nullptr;

    void writeU32LE(uint32_t v);
    void writeU16LE(uint16_t v);
    void writeFourCC(const char* cc);
    void buildAviHeader(uint8_t* buf, uint32_t w, uint32_t h,
                        uint32_t frames, uint32_t moviData);
};
