#include "video_recorder.h"

// ─── Constructor / destructor ─────────────────────────────────────────────────

VideoRecorder::VideoRecorder() {
    _index = new AviIdx[MAX_INDEX];
}

VideoRecorder::~VideoRecorder() {
    if (_recording) finish();
    delete[] _index;
}

// ─── Schrijfhelpers ───────────────────────────────────────────────────────────

static void writeU32LE_buf(uint8_t* p, uint32_t v) {
    p[0] = v & 0xFF; p[1] = (v>>8)&0xFF; p[2] = (v>>16)&0xFF; p[3] = (v>>24)&0xFF;
}
static void writeU16LE_buf(uint8_t* p, uint16_t v) {
    p[0] = v & 0xFF; p[1] = (v>>8)&0xFF;
}

void VideoRecorder::writeU32LE(uint32_t v) {
    uint8_t b[4]; writeU32LE_buf(b, v); _file.write(b, 4);
}
void VideoRecorder::writeU16LE(uint16_t v) {
    uint8_t b[2]; writeU16LE_buf(b, v); _file.write(b, 2);
}
void VideoRecorder::writeFourCC(const char* cc) {
    _file.write((const uint8_t*)cc, 4);
}

// ─── AVI header opbouwen in een 248-byte buffer ───────────────────────────────
//
// Vaste layout (248 bytes totaal):
//   [  0] RIFF ????  AVI     12
//   [ 12] LIST ????  hdrl    12
//   [ 24]   avih(56)         64   → totalFrames @ [84]
//   [ 88] LIST ????  strl    12
//   [100]   strh(56)         64   → length @ [196]
//   [164]   strf(40)         48
//   [212] JUNK(28)           36   ← padding zodat movi op 248 valt
//   [240] LIST ????  movi     8   → movi-size @ [244]
//   [248] <frames starten hier>
//
void VideoRecorder::buildAviHeader(uint8_t* buf, uint32_t w, uint32_t h,
                                    uint32_t totalFrames, uint32_t moviDataSize) {
    memset(buf, 0, HEADER_SIZE);
    uint32_t fps = CAM_FPS_TARGET;
    uint32_t moviListSize = 4 + moviDataSize;   // "movi" + frames
    uint32_t idxSize = 8 + totalFrames * 16;    // "idx1" + entries
    uint32_t riffSize = HEADER_SIZE - 8         // alles na RIFF header
                      + moviDataSize
                      + idxSize;

    // RIFF
    memcpy(buf+0,  "RIFF", 4); writeU32LE_buf(buf+4,  riffSize);
    memcpy(buf+8,  "AVI ", 4);

    // LIST hdrl
    memcpy(buf+12, "LIST", 4); writeU32LE_buf(buf+16, 212);
    memcpy(buf+20, "hdrl", 4);

    // avih
    memcpy(buf+24, "avih", 4); writeU32LE_buf(buf+28, 56);
    writeU32LE_buf(buf+32, 1000000 / fps);   // microSecPerFrame
    writeU32LE_buf(buf+36, 0);               // maxBytesPerSec
    writeU32LE_buf(buf+40, 0);               // paddingGranularity
    writeU32LE_buf(buf+44, 0x10);            // AVIF_HASINDEX
    writeU32LE_buf(buf+48, totalFrames);     // totalFrames
    writeU32LE_buf(buf+52, 0);               // initialFrames
    writeU32LE_buf(buf+56, 1);               // streams
    writeU32LE_buf(buf+60, w*h*3);           // suggestedBufferSize
    writeU32LE_buf(buf+64, w);
    writeU32LE_buf(buf+68, h);

    // LIST strl
    memcpy(buf+88,  "LIST", 4); writeU32LE_buf(buf+92,  132);
    memcpy(buf+96,  "strl", 4);

    // strh
    memcpy(buf+100, "strh", 4); writeU32LE_buf(buf+104, 56);
    memcpy(buf+108, "vids", 4);
    memcpy(buf+112, "MJPG", 4);
    writeU32LE_buf(buf+116, 0);              // flags
    writeU16LE_buf(buf+120, 0);              // priority
    writeU16LE_buf(buf+122, 0);              // language
    writeU32LE_buf(buf+124, 0);              // initialFrames
    writeU32LE_buf(buf+128, 1);              // scale
    writeU32LE_buf(buf+132, fps);            // rate
    writeU32LE_buf(buf+136, 0);              // start
    writeU32LE_buf(buf+140, totalFrames);    // length
    writeU32LE_buf(buf+144, w*h*3);          // suggestedBufferSize
    writeU32LE_buf(buf+148, 0xFFFFFFFF);     // quality
    writeU32LE_buf(buf+152, 0);              // sampleSize
    writeU16LE_buf(buf+160, (uint16_t)w);
    writeU16LE_buf(buf+162, (uint16_t)h);

    // strf BITMAPINFOHEADER
    memcpy(buf+164, "strf", 4); writeU32LE_buf(buf+168, 40);
    writeU32LE_buf(buf+172, 40);             // biSize
    writeU32LE_buf(buf+176, w);
    writeU32LE_buf(buf+180, h);
    writeU16LE_buf(buf+184, 1);              // biPlanes
    writeU16LE_buf(buf+186, 24);             // biBitCount
    memcpy(buf+188, "MJPG", 4);              // biCompression
    writeU32LE_buf(buf+192, w*h*3);          // biSizeImage

    // JUNK padding: bytes 212..239
    memcpy(buf+212, "JUNK", 4); writeU32LE_buf(buf+216, 20);
    // 20 bytes payload (nullen) → JUNK chunk = 28 bytes totaal → positie 240

    // LIST movi
    memcpy(buf+240, "LIST", 4); writeU32LE_buf(buf+244, moviListSize);
    memcpy(buf+248-4, "movi", 4);  // offset 244: "movi" fourcc
    // Frames starten op offset 248
}

// ─── Publieke interface ────────────────────────────────────────────────────────

bool VideoRecorder::begin(const char* filename) {
    if (_recording) finish();

    strncpy(_filename, filename, sizeof(_filename) - 1);
    _filename[sizeof(_filename) - 1] = '\0';
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

    // Schrijf placeholder header (totalFrames=0, moviSize=0)
    // Na finish() wordt het bestand opnieuw geopend en de header overschreven
    uint8_t hdr[HEADER_SIZE];
    buildAviHeader(hdr, CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT, 0, 0);
    size_t written = _file.write(hdr, HEADER_SIZE);

    if (written != HEADER_SIZE || _file.position() != HEADER_SIZE) {
#if DEBUG_SERIAL
        Serial.printf("[Recorder] Header schrijven mislukt (geschreven: %u)\n", written);
#endif
        _file.close();
        SD_MMC.remove(filename);
        return false;
    }

    _recording = true;
#if DEBUG_SERIAL
    Serial.printf("[Recorder] Opname gestart: %s\n", filename);
#endif
    return true;
}

bool VideoRecorder::writeFrame(const uint8_t* data, size_t len) {
    if (!_recording || !_file) return false;
    if (len == 0) return false;

    uint32_t frameOffset = (uint32_t)_file.position() - HEADER_SIZE;

    writeFourCC("00dc");
    writeU32LE((uint32_t)len);
    size_t written = _file.write(data, len);

    if (written != len) {
#if DEBUG_SERIAL
        Serial.printf("[Recorder] SD schrijven mislukt! (gevraagd=%u geschreven=%u)\n",
                      len, written);
#endif
        return false;
    }

    if (len & 1) {
        uint8_t pad = 0;
        _file.write(&pad, 1);
    }

    if (_frameCount < MAX_INDEX) {
        _index[_frameCount] = { frameOffset, (uint32_t)len };
    }

    _moviSize += 8 + (uint32_t)len + (len & 1);
    _frameCount++;

#if DEBUG_SERIAL
    if (_frameCount % 30 == 0) {
        Serial.printf("[Recorder] %u frames, %u bytes movi\n", _frameCount, _moviSize);
    }
#endif

    return true;
}

bool VideoRecorder::finish() {
    if (!_recording || !_file) return false;
    _recording = false;

    // ── idx1 schrijven ──────────────────────────────────────────────────────
    uint32_t idxCount = (_frameCount < MAX_INDEX) ? _frameCount : MAX_INDEX;
    writeFourCC("idx1");
    writeU32LE(idxCount * 16);
    for (uint32_t i = 0; i < idxCount; i++) {
        writeFourCC("00dc");
        writeU32LE(0x10);
        writeU32LE(_index[i].offset);
        writeU32LE(_index[i].size);
    }

    _file.close();

    // ── Header patchen door bestand opnieuw te openen ──────────────────────
    // SD_MMC ondersteunt geen betrouwbare seek+write in open write-bestanden.
    // Oplossing: open met r+ (FILE_WRITE opent voor append op ESP32,
    // maar we kunnen het bestand opnieuw openen en de header overschrijven).
    File patch = SD_MMC.open(_filename, FILE_WRITE);
    if (patch) {
        uint8_t hdr[HEADER_SIZE];
        buildAviHeader(hdr, CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT, _frameCount, _moviSize);
        patch.write(hdr, HEADER_SIZE);
        patch.close();
#if DEBUG_SERIAL
        Serial.printf("[Recorder] Header gepatcht: %u frames, %u bytes movi\n",
                      _frameCount, _moviSize);
#endif
    } else {
#if DEBUG_SERIAL
        Serial.println("[Recorder] Header patchen mislukt (bestand niet te openen)");
#endif
    }

    uint32_t elapsed = (millis() - _startMs) / 1000;
#if DEBUG_SERIAL
    Serial.printf("[Recorder] Klaar: %u frames in %u sec → %s\n",
                  _frameCount, elapsed, _filename);
#endif
    return true;
}
