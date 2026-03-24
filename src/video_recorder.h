#pragma once
#include <Arduino.h>
#include <SD_MMC.h>
#include "config.h"

class VideoRecorder {
public:
    VideoRecorder();
    ~VideoRecorder();

    bool begin(const char* filename, uint32_t frameWidth, uint32_t frameHeight);
    bool writeFrame(const uint8_t* data, size_t len);
    bool finish();

    bool        isRecording() const { return _recording; }
    const char* currentFile() const { return _filename; }
    uint32_t    frameCount()  const { return _frameCount; }

    // Header layout (bytes):
    //   [  0] RIFF + AVI                   12
    //   [ 12] LIST hdrl (192)             212   → ends at 211
    //   [212] JUNK (28)                    28   → ends at 239
    //   [240] LIST movi header             12   ("LIST"+size+"movi")
    //   [252] frames starten hier
    static const uint32_t HEADER_SIZE = 252;

private:
    File     _file;
    char     _filename[64];
    char     _vfsPath[80];
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
