#include "video_recorder.h"

// ─── AVI hulpfuncties ─────────────────────────────────────────────────────────

static void writeU32LE(File& f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)(v), (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24) };
    f.write(b, 4);
}
static void writeU16LE(File& f, uint16_t v) {
    uint8_t b[2] = { (uint8_t)(v), (uint8_t)(v>>8) };
    f.write(b, 2);
}
static void writeFourCC(File& f, const char* cc) {
    f.write((const uint8_t*)cc, 4);
}

// ─── AVI header schrijven (placeholder, wordt achteraf bijgewerkt) ────────────
void VideoRecorder::writeAviHeader(uint32_t width, uint32_t height) {
    _width  = width;
    _height = height;

    // RIFF AVI header – totaal 240 bytes (vaste grootte)
    // Waarden die afhangen van frame count worden als 0 ingevuld
    // en later bijgewerkt via finalizeAviHeader()

    writeFourCC(_file, "RIFF");
    writeU32LE(_file, 0);              // [4]  RIFF size – later bijwerken
    writeFourCC(_file, "AVI ");

    // ── LIST hdrl ────────────────────────────────────────────────────────────
    writeFourCC(_file, "LIST");
    writeU32LE(_file, 192);            // hdrl list size (vast)
    writeFourCC(_file, "hdrl");

    // avih
    writeFourCC(_file, "avih");
    writeU32LE(_file, 56);
    writeU32LE(_file, 1000000 / CAM_FPS_TARGET); // microSecPerFrame
    writeU32LE(_file, 0);              // maxBytesPerSec
    writeU32LE(_file, 0);              // paddingGranularity
    writeU32LE(_file, 0x10);           // flags: AVIF_HASINDEX
    writeU32LE(_file, 0);              // [84] totalFrames – later bijwerken
    writeU32LE(_file, 0);              // initialFrames
    writeU32LE(_file, 1);              // streams
    writeU32LE(_file, 0);              // suggestedBufferSize
    writeU32LE(_file, width);
    writeU32LE(_file, height);
    writeU32LE(_file, 0); writeU32LE(_file, 0);
    writeU32LE(_file, 0); writeU32LE(_file, 0);

    // LIST strl
    writeFourCC(_file, "LIST");
    writeU32LE(_file, 116);
    writeFourCC(_file, "strl");

    // strh
    writeFourCC(_file, "strh");
    writeU32LE(_file, 56);
    writeFourCC(_file, "vids");
    writeFourCC(_file, "MJPG");
    writeU32LE(_file, 0);              // flags
    writeU16LE(_file, 0);              // priority
    writeU16LE(_file, 0);              // language
    writeU32LE(_file, 0);              // initialFrames
    writeU32LE(_file, 1);              // scale
    writeU32LE(_file, CAM_FPS_TARGET); // rate
    writeU32LE(_file, 0);              // start
    writeU32LE(_file, 0);             // [204] length – later bijwerken
    writeU32LE(_file, 0);              // suggestedBufferSize
    writeU32LE(_file, 0);              // quality
    writeU32LE(_file, 0);              // sampleSize
    writeU16LE(_file, 0); writeU16LE(_file, 0);
    writeU16LE(_file, (uint16_t)width);
    writeU16LE(_file, (uint16_t)height);

    // strf (BITMAPINFOHEADER)
    writeFourCC(_file, "strf");
    writeU32LE(_file, 40);
    writeU32LE(_file, 40);
    writeU32LE(_file, width);
    writeU32LE(_file, height);
    writeU16LE(_file, 1);              // planes
    writeU16LE(_file, 24);             // bitCount
    writeFourCC(_file, "MJPG");
    writeU32LE(_file, width * height * 3);
    writeU32LE(_file, 0); writeU32LE(_file, 0);
    writeU32LE(_file, 0); writeU32LE(_file, 0);

    // Junk chunk om movi op offset 240 te starten
    writeFourCC(_file, "JUNK");
    writeU32LE(_file, 4);
    writeU32LE(_file, 0);

    // LIST movi
    writeFourCC(_file, "LIST");
    writeU32LE(_file, 0);              // [236] movi size – later bijwerken
    writeFourCC(_file, "movi");

    _dataStart = 240; // movi data begint na header
}

void VideoRecorder::finalizeAviHeader() {
    uint32_t moviListSize = _moviSize + 4; // +4 voor "movi" fourcc

    // RIFF chunk grootte
    _file.seek(4);
    writeU32LE(_file, 240 - 8 + moviListSize + 8 + _frameCount * 16); // approx

    // totalFrames in avih
    _file.seek(84);
    writeU32LE(_file, _frameCount);

    // length in strh
    _file.seek(204);
    writeU32LE(_file, _frameCount);

    // movi LIST grootte
    _file.seek(236);
    writeU32LE(_file, moviListSize);
}

void VideoRecorder::writeIndex() {
    writeFourCC(_file, "idx1");
    writeU32LE(_file, _frameCount * 16);

    for (uint32_t i = 0; i < _frameCount && i < MAX_INDEX; i++) {
        writeFourCC(_file, "00dc");
        writeU32LE(_file, 0x10);       // AVIIF_KEYFRAME
        writeU32LE(_file, _index[i].offset);
        writeU32LE(_file, _index[i].size);
    }
}

// ─── Publieke interface ────────────────────────────────────────────────────────

bool VideoRecorder::begin(const char* filename) {
    if (_recording) finish();

    strncpy(_filename, filename, sizeof(_filename) - 1);
    _frameCount = 0;
    _moviSize   = 0;
    _startMs    = millis();

    _file = SD_MMC.open(filename, FILE_WRITE);
    if (!_file) {
#if DEBUG_SERIAL
        Serial.printf("[Recorder] Kan bestand niet openen: %s\n", filename);
#endif
        return false;
    }

    // Tijdelijke header schrijven; dimensies onbekend tot eerste frame
    // Schrijf 240 nul-bytes als placeholder
    uint8_t zeros[240] = {};
    _file.write(zeros, 240);

    _recording = true;
#if DEBUG_SERIAL
    Serial.printf("[Recorder] Opname gestart: %s\n", filename);
#endif
    return true;
}

bool VideoRecorder::writeFrame(const uint8_t* data, size_t len) {
    if (!_recording || !_file) return false;
    if (_frameCount >= MAX_INDEX) return false;

    // Eerste frame: header terugschrijven met juiste dimensies
    // Dimensies zijn ingebed in JPEG; we gebruiken standaard VGA als default
    if (_frameCount == 0) {
        _file.seek(0);
        writeAviHeader(CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT);
        _file.seek(0, SeekEnd);
    }

    // Bewaar offset relatief aan movi start (na "movi" fourcc op offset 244)
    uint32_t frameOffset = (uint32_t)_file.position() - _dataStart - 4;

    writeFourCC(_file, "00dc");
    writeU32LE(_file, (uint32_t)len);
    _file.write(data, len);
    // Padding op even grens
    if (len & 1) _file.write((uint8_t)0);

    if (_frameCount < MAX_INDEX) {
        _index[_frameCount] = { frameOffset, (uint32_t)len };
    }

    uint32_t chunkSize = 8 + len + (len & 1);
    _moviSize += chunkSize;
    _frameCount++;

    return true;
}

bool VideoRecorder::finish() {
    if (!_recording || !_file) return false;
    _recording = false;

    // Schrijf index
    writeIndex();

    // Bijwerken van header velden
    finalizeAviHeader();

    _file.close();

    uint32_t elapsed = (millis() - _startMs) / 1000;
#if DEBUG_SERIAL
    Serial.printf("[Recorder] Opname gestopt: %u frames in %u sec -> %s\n",
                  _frameCount, elapsed, _filename);
#endif
    return true;
}
