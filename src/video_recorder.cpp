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

void VideoRecorder::writeU32LE(uint32_t v) {
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v>>8), (uint8_t)(v>>16), (uint8_t)(v>>24) };
    _file.write(b, 4);
}

void VideoRecorder::writeU16LE(uint16_t v) {
    uint8_t b[2] = { (uint8_t)v, (uint8_t)(v>>8) };
    _file.write(b, 2);
}

void VideoRecorder::writeFourCC(const char* cc) {
    _file.write((const uint8_t*)cc, 4);
}

// Patch 4 bytes op een willekeurige positie in het bestand.
// Slaat de huidige positie op en herstelt deze daarna.
void VideoRecorder::patchU32(uint32_t fileOffset, uint32_t value) {
    uint32_t saved = (uint32_t)_file.position();
    _file.seek(fileOffset);
    writeU32LE(value);
    _file.seek(saved);
}

// ─── AVI header (vaste grootte 244 bytes) ────────────────────────────────────
//
// Byte-map (gebruikt voor patch-adressen):
//   [4]   RIFF size
//  [84]   avih.totalFrames
// [136]   strh.length
// [236]   LIST movi size
//
// Structuur:
//   RIFF(AVI )                              12
//     LIST(hdrl)                            12
//       avih(56)                            64   → offset 12
//       LIST(strl)                          12
//         strh(56)                          64   → offset 104
//         strf(40) BITMAPINFOHEADER         48   → offset 168
//     JUNK(4)                               12   → offset 228 (padding tot 244)
//     LIST(movi)                             8   → offset 240
//       <frames starten op offset 248>
//
void VideoRecorder::writeAviHeader(uint32_t w, uint32_t h) {
    // RIFF AVI
    writeFourCC("RIFF");
    writeU32LE(0);                        // [4]  patch later
    writeFourCC("AVI ");

    // LIST hdrl
    writeFourCC("LIST");
    writeU32LE(212);                      // hdrl list grootte (vast)
    writeFourCC("hdrl");

    // avih  – offset 24
    writeFourCC("avih");
    writeU32LE(56);                       // chunk size
    writeU32LE(1000000 / CAM_FPS_TARGET); // microSecPerFrame
    writeU32LE(0);                        // maxBytesPerSec
    writeU32LE(0);                        // paddingGranularity
    writeU32LE(0x10);                     // flags AVIF_HASINDEX
    writeU32LE(0);                        // [84] totalFrames  – patch later
    writeU32LE(0);                        // initialFrames
    writeU32LE(1);                        // streams
    writeU32LE(w * h * 3);               // suggestedBufferSize
    writeU32LE(w);                        // width
    writeU32LE(h);                        // height
    writeU32LE(0); writeU32LE(0);
    writeU32LE(0); writeU32LE(0);

    // LIST strl – offset 96
    writeFourCC("LIST");
    writeU32LE(132);
    writeFourCC("strl");

    // strh – offset 108
    writeFourCC("strh");
    writeU32LE(56);
    writeFourCC("vids");
    writeFourCC("MJPG");
    writeU32LE(0);                        // flags
    writeU16LE(0);                        // priority
    writeU16LE(0);                        // language
    writeU32LE(0);                        // initialFrames
    writeU32LE(1);                        // scale
    writeU32LE(CAM_FPS_TARGET);           // rate  (fps)
    writeU32LE(0);                        // start
    writeU32LE(0);                        // [136+8+56-4=196] length  – patch later
    writeU32LE(w * h * 3);               // suggestedBufferSize
    writeU32LE(0xFFFFFFFF);              // quality
    writeU32LE(0);                        // sampleSize
    writeU16LE(0); writeU16LE(0);
    writeU16LE((uint16_t)w);
    writeU16LE((uint16_t)h);

    // strf (BITMAPINFOHEADER) – offset 172
    writeFourCC("strf");
    writeU32LE(40);
    writeU32LE(40);                       // biSize
    writeU32LE(w);
    writeU32LE(h);
    writeU16LE(1);                        // biPlanes
    writeU16LE(24);                       // biBitCount
    writeFourCC("MJPG");                  // biCompression
    writeU32LE(w * h * 3);               // biSizeImage
    writeU32LE(0); writeU32LE(0);
    writeU32LE(0); writeU32LE(0);

    // JUNK chunk – padding tot offset 240
    // Huidige positie: 12+8+12+64+8+8+64+8+48 = 232 bytes
    writeFourCC("JUNK");
    writeU32LE(4);
    writeU32LE(0);                        // 4 bytes payload → totaal 240

    // LIST movi – offset 240
    writeFourCC("LIST");
    writeU32LE(0);                        // [244] movi data size – patch later
    writeFourCC("movi");
    // Frames beginnen op offset 248
}

// ─── Patch-offsets (berekend op basis van header layout hierboven) ────────────
static const uint32_t OFF_RIFF_SIZE    =   4;
static const uint32_t OFF_TOTAL_FRAMES =  84;
static const uint32_t OFF_STRH_LENGTH  = 196;
static const uint32_t OFF_MOVI_SIZE    = 244;

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

    // Schrijf volledige header met echte dimensies direct.
    // Framecount-velden worden later gepatcht via patchU32().
    writeAviHeader(CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT);

    // Controleer dat we op offset 248 staan
    if (_file.position() != 248) {
#if DEBUG_SERIAL
        Serial.printf("[Recorder] Header grootte fout: %u bytes (verwacht 248)\n",
                      (uint32_t)_file.position());
#endif
        _file.close();
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

    // Offset relatief aan movi-begin (offset 248) min 4 bytes voor "movi" fourcc
    // De idx1 offset is relatief aan het begin van de movi LIST data,
    // dat is 4 bytes na de "LIST" size veld, dus offset 248 (na de "movi" fourcc).
    // Conventie: offset is relatief aan start van de movi data (byte 248).
    uint32_t frameOffset = (uint32_t)_file.position() - 248;

    writeFourCC("00dc");
    writeU32LE((uint32_t)len);
    _file.write(data, len);
    if (len & 1) {
        uint8_t pad = 0;
        _file.write(&pad, 1);
    }

    if (_frameCount < MAX_INDEX) {
        _index[_frameCount] = { frameOffset, (uint32_t)len };
    }

    _moviSize += 8 + (uint32_t)len + (len & 1);
    _frameCount++;

    return true;
}

bool VideoRecorder::finish() {
    if (!_recording || !_file) return false;
    _recording = false;

    // ── idx1 chunk schrijven ────────────────────────────────────────────────
    uint32_t idxCount = (_frameCount < MAX_INDEX) ? _frameCount : MAX_INDEX;
    writeFourCC("idx1");
    writeU32LE(idxCount * 16);
    for (uint32_t i = 0; i < idxCount; i++) {
        writeFourCC("00dc");
        writeU32LE(0x10);                  // AVIIF_KEYFRAME
        writeU32LE(_index[i].offset);
        writeU32LE(_index[i].size);
    }

    // ── Header patchen ──────────────────────────────────────────────────────
    uint32_t idxSize   = 8 + idxCount * 16;
    uint32_t riffSize  = 248 - 8 + _moviSize + 8 + idxSize;
    //                   ^248 = LIST hdrl + LIST movi header (240+8)
    //                   subtract 8 for the RIFF header itself

    patchU32(OFF_RIFF_SIZE,    riffSize);
    patchU32(OFF_TOTAL_FRAMES, _frameCount);
    patchU32(OFF_STRH_LENGTH,  _frameCount);
    // movi LIST size = 4 (voor "movi" fourcc) + alle frame chunks
    patchU32(OFF_MOVI_SIZE,    4 + _moviSize);

    _file.close();

    uint32_t elapsed = (millis() - _startMs) / 1000;
#if DEBUG_SERIAL
    Serial.printf("[Recorder] Klaar: %u frames, %u sec, %u bytes movi → %s\n",
                  _frameCount, elapsed, _moviSize, _filename);
#endif
    return true;
}
