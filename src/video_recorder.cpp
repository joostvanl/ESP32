#include "video_recorder.h"
#include <stdio.h>

// ─── Constructor / destructor ─────────────────────────────────────────────────

VideoRecorder::VideoRecorder() {
    _index = new AviIdx[MAX_INDEX];
}

VideoRecorder::~VideoRecorder() {
    if (_recording) finish();
    delete[] _index;
}

// ─── Byte-schrijfhelpers ──────────────────────────────────────────────────────

static void u32le(uint8_t* p, uint32_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
    p[2]=(uint8_t)(v>>16); p[3]=(uint8_t)(v>>24);
}
static void u16le(uint8_t* p, uint16_t v) {
    p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8);
}

void VideoRecorder::writeU32LE(uint32_t v) {
    uint8_t b[4]; u32le(b,v); _file.write(b,4);
}
void VideoRecorder::writeU16LE(uint16_t v) {
    uint8_t b[2]; u16le(b,v); _file.write(b,2);
}
void VideoRecorder::writeFourCC(const char* cc) {
    _file.write((const uint8_t*)cc, 4);
}

// ─── AVI header bouwen (HEADER_SIZE = 252 bytes) ──────────────────────────────
//
// Exacte byte-layout:
//
//  Offset  Inhoud                              Grootte
//  ──────  ──────────────────────────────────  ───────
//    0     "RIFF"                                4
//    4     riff_size (gepatcht in finish)         4
//    8     "AVI "                                4
//  ── LIST hdrl (200 bytes totaal) ─────────────────────
//   12     "LIST"                                4
//   16     192 (hdrl inhoud)                     4
//   20     "hdrl"                                4
//  ── avih (64 bytes) ──────────────────────────────────
//   24     "avih"  56                             8
//   32     microSecPerFrame                       4
//   36     0 maxBytesPerSec                       4
//   40     0 paddingGranularity                   4
//   44     0x10 flags (AVIF_HASINDEX)             4
//   48     totalFrames (gepatcht)                 4
//   52     0 initialFrames                        4
//   56     1 streams                              4
//   60     suggestedBufferSize                    4
//   64     width                                  4
//   68     height                                 4
//   72     0×16 reserved                         16
//  ── LIST strl (124 bytes totaal) ─────────────────────
//   88     "LIST"                                4
//   92     116 (strl inhoud = "strl"+strh+strf)  4
//   96     "strl"                                4
//  ── strh (64 bytes) ──────────────────────────────────
//  100     "strh"  56                             8
//  108     "vids"                                4
//  112     "MJPG"                                4
//  116     0 flags                               4
//  120     0 priority / 0 language               4
//  124     0 initialFrames                       4
//  128     1 scale                               4
//  132     fps rate                              4
//  136     0 start                               4
//  140     totalFrames length (gepatcht)         4
//  144     suggestedBufferSize                   4
//  148     0xFFFFFFFF quality                    4
//  152     0 sampleSize                          4
//  156     0 left / 0 top                        4
//  160     width right / height bottom           4
//  ── strf / BITMAPINFOHEADER (48 bytes) ───────────────
//  164     "strf"  40                             8
//  172     40 biSize                             4
//  176     width                                 4
//  180     height                                4
//  184     1 biPlanes / 24 biBitCount            4
//  188     "MJPG"                                4
//  192     biSizeImage                           4
//  196     0×16 (XPels, YPels, ClrUsed, ClrImp) 16
//  ── JUNK (28 bytes totaal, eindigt op 239) ───────────
//  212     "JUNK"  20                             8
//  220     0×20 payload                          20
//  ── LIST movi header (12 bytes, eindigt op 251) ──────
//  240     "LIST"                                4
//  244     movi_size (gepatcht)                  4
//  248     "movi"                                4
//  ── frames starten op 252 = HEADER_SIZE ──────────────
//
void VideoRecorder::buildAviHeader(uint8_t* b, uint32_t w, uint32_t h,
                                    uint32_t frames, uint32_t moviData) {
    memset(b, 0, HEADER_SIZE);

    const uint32_t fps   = CAM_FPS_TARGET;
    const uint32_t idxSz = 8 + frames * 16;
    // RIFF size = alles ná de eerste 8 bytes
    const uint32_t riffSz = (HEADER_SIZE - 8) + moviData + idxSz;
    // movi LIST size = "movi"(4) + alle frame-chunks
    const uint32_t moviSz = 4 + moviData;

    // RIFF
    memcpy(b+0,  "RIFF", 4);  u32le(b+4,  riffSz);
    memcpy(b+8,  "AVI ", 4);

    // LIST hdrl (size = 192)
    memcpy(b+12, "LIST", 4);  u32le(b+16, 192);
    memcpy(b+20, "hdrl", 4);

    // avih @ 24
    memcpy(b+24, "avih", 4);  u32le(b+28, 56);
    u32le(b+32, 1000000 / fps); // microSecPerFrame
    u32le(b+36, 0);
    u32le(b+40, 0);
    u32le(b+44, 0x10);          // AVIF_HASINDEX
    u32le(b+48, frames);        // totalFrames
    u32le(b+52, 0);
    u32le(b+56, 1);             // streams
    u32le(b+60, w * h * 3);     // suggestedBufferSize
    u32le(b+64, w);
    u32le(b+68, h);
    // b[72..87] = 0 (reserved, al gezet door memset)

    // LIST strl @ 88 (size = 116)
    memcpy(b+88, "LIST", 4);  u32le(b+92, 116);
    memcpy(b+96, "strl", 4);

    // strh @ 100
    memcpy(b+100, "strh", 4);  u32le(b+104, 56);
    memcpy(b+108, "vids", 4);
    memcpy(b+112, "MJPG", 4);
    u32le(b+116, 0);            // flags
    u16le(b+120, 0);            // priority
    u16le(b+122, 0);            // language
    u32le(b+124, 0);            // initialFrames
    u32le(b+128, 1);            // scale
    u32le(b+132, fps);          // rate
    u32le(b+136, 0);            // start
    u32le(b+140, frames);       // length
    u32le(b+144, w * h * 3);    // suggestedBufferSize
    u32le(b+148, 0xFFFFFFFF);   // quality (-1 = default)
    u32le(b+152, 0);            // sampleSize
    u16le(b+156, 0);            // rcFrame.left
    u16le(b+158, 0);            // rcFrame.top
    u16le(b+160, (uint16_t)w);  // rcFrame.right
    u16le(b+162, (uint16_t)h);  // rcFrame.bottom

    // strf @ 164 (BITMAPINFOHEADER, size = 40)
    memcpy(b+164, "strf", 4);  u32le(b+168, 40);
    u32le(b+172, 40);           // biSize
    u32le(b+176, w);            // biWidth
    u32le(b+180, h);            // biHeight
    u16le(b+184, 1);            // biPlanes
    u16le(b+186, 24);           // biBitCount
    memcpy(b+188, "MJPG", 4);   // biCompression
    u32le(b+192, w * h * 3);    // biSizeImage
    // b[196..211] = 0

    // JUNK @ 212 (payload=20, totaal chunk=28, eindigt op byte 239)
    memcpy(b+212, "JUNK", 4);  u32le(b+216, 20);
    // b[220..239] = 0

    // LIST movi @ 240 ("LIST" + size + "movi", 12 bytes, eindigt op 251)
    memcpy(b+240, "LIST", 4);  u32le(b+244, moviSz);
    memcpy(b+248, "movi", 4);
    // Frames starten op byte 252 = HEADER_SIZE
}

// ─── Opname starten ───────────────────────────────────────────────────────────

bool VideoRecorder::begin(const char* filename, uint32_t frameWidth, uint32_t frameHeight) {
    if (_recording) finish();

    strncpy(_filename, filename, sizeof(_filename)-1);
    _filename[sizeof(_filename)-1] = '\0';
    _frameCount = 0;
    _moviSize   = 0;
    _startMs    = millis();

    Serial.printf("[REC] begin() → %s (%ux%u)\n", _filename,
                  (unsigned)frameWidth, (unsigned)frameHeight);
    Serial.printf("[REC] Vrij heap: %u bytes\n", (uint32_t)ESP.getFreeHeap());

    snprintf(_vfsPath, sizeof(_vfsPath), "/sdcard%s", _filename);
    Serial.printf("[REC] VFS pad voor fopen: %s\n", _vfsPath);

    _file = SD_MMC.open(filename, FILE_WRITE);
    if (!_file) {
        Serial.printf("[REC] FOUT: SD_MMC.open mislukt: %s\n", filename);
        return false;
    }
    Serial.printf("[REC] Bestand geopend, positie=%u\n", (uint32_t)_file.position());

    uint8_t hdr[HEADER_SIZE];
    buildAviHeader(hdr, frameWidth, frameHeight, 0, 0);
    size_t wr = _file.write(hdr, HEADER_SIZE);

    Serial.printf("[REC] Header geschreven: %u/%u bytes, positie=%u\n",
                  wr, (uint32_t)HEADER_SIZE, (uint32_t)_file.position());

    if (wr != HEADER_SIZE || _file.position() != HEADER_SIZE) {
        Serial.println("[REC] FOUT: Header schrijven mislukt!");
        _file.close();
        SD_MMC.remove(filename);
        return false;
    }

    _recording = true;
    Serial.println("[REC] Opname gestart OK");
    return true;
}

// ─── Frame schrijven ──────────────────────────────────────────────────────────

bool VideoRecorder::writeFrame(const uint8_t* data, size_t len) {
    if (!_recording || !_file) {
        Serial.println("[REC] writeFrame: niet actief");
        return false;
    }
    if (len == 0) {
        Serial.println("[REC] writeFrame: len=0, overgeslagen");
        return false;
    }

    // Offset relatief aan "movi" data (byte HEADER_SIZE)
    uint32_t posBefore   = (uint32_t)_file.position();
    uint32_t frameOffset = posBefore - HEADER_SIZE;

    writeFourCC("00dc");
    writeU32LE((uint32_t)len);
    size_t wr = _file.write(data, len);

    if (wr != len) {
        Serial.printf("[REC] FOUT frame #%u: gevraagd=%u geschreven=%u\n",
                      _frameCount+1, (uint32_t)len, (uint32_t)wr);
        return false;
    }

    if (len & 1) { uint8_t pad=0; _file.write(&pad, 1); }

    if (_frameCount < MAX_INDEX) {
        _index[_frameCount] = { frameOffset, (uint32_t)len };
    }
    _moviSize += 8 + (uint32_t)len + (len & 1);
    _frameCount++;

    if (_frameCount == 1 || _frameCount % 10 == 0) {
        Serial.printf("[REC] Frame #%u: %u bytes, movi=%u, pos=%u\n",
                      _frameCount, (uint32_t)len, _moviSize, (uint32_t)_file.position());
    }
    return true;
}

// ─── Opname stoppen ───────────────────────────────────────────────────────────

bool VideoRecorder::finish() {
    if (!_recording || !_file) {
        Serial.println("[REC] finish() – geen actieve opname");
        return false;
    }
    _recording = false;

    Serial.printf("[REC] finish() – %u frames, moviSize=%u\n", _frameCount, _moviSize);

    // idx1 index schrijven
    uint32_t idxCount = (_frameCount < MAX_INDEX) ? _frameCount : MAX_INDEX;
    writeFourCC("idx1");
    writeU32LE(idxCount * 16);
    for (uint32_t i = 0; i < idxCount; i++) {
        writeFourCC("00dc");
        writeU32LE(0x10);               // AVIIF_KEYFRAME
        writeU32LE(_index[i].offset);
        writeU32LE(_index[i].size);
    }
    Serial.printf("[REC] idx1 geschreven, eindpositie=%u\n", (uint32_t)_file.position());
    _file.close();

    // Verificatie vóór patch
    {
        File v = SD_MMC.open(_filename, FILE_READ);
        if (v) { Serial.printf("[REC] Grootte vóór patch: %u bytes\n", (uint32_t)v.size()); v.close(); }
        else { Serial.println("[REC] FOUT: bestand niet leesbaar na close!"); return false; }
    }

    // Header patchen via fopen r+b (schrijft zonder truncate)
    // FILE_WRITE in ESP32 Arduino = "w" = truncate → gebruikt fopen r+b i.p.v.
    Serial.printf("[REC] Patch via fopen r+b: %s\n", _vfsPath);
    FILE* fp = fopen(_vfsPath, "r+b");
    if (!fp) {
        Serial.printf("[REC] fopen r+b mislukt (errno=%d), probeer zonder /sdcard\n", errno);
        fp = fopen(_filename, "r+b");
        if (!fp) {
            Serial.printf("[REC] FOUT: fopen mislukt (errno=%d)\n", errno);
            return false;
        }
        Serial.println("[REC] Pad zonder /sdcard werkt");
    }

    uint8_t tmp[4];
    const uint32_t idxSz  = 8 + idxCount * 16;
    const uint32_t riffSz = (HEADER_SIZE - 8) + _moviSize + idxSz;
    const uint32_t moviSz = 4 + _moviSize;

    u32le(tmp, riffSz);    fseek(fp,   4, SEEK_SET); fwrite(tmp, 1, 4, fp); // RIFF size
    u32le(tmp, _frameCount); fseek(fp, 48, SEEK_SET); fwrite(tmp, 1, 4, fp); // totalFrames
    u32le(tmp, _frameCount); fseek(fp,140, SEEK_SET); fwrite(tmp, 1, 4, fp); // strh length
    u32le(tmp, moviSz);    fseek(fp, 244, SEEK_SET); fwrite(tmp, 1, 4, fp); // movi LIST size
    fclose(fp);

    Serial.println("[REC] Patch klaar");

    // Finale verificatie
    {
        File v = SD_MMC.open(_filename, FILE_READ);
        if (v) {
            uint32_t sz = (uint32_t)v.size();
            uint8_t magic[4]; v.read(magic, 4); v.close();
            Serial.printf("[REC] Finale grootte: %u bytes\n", sz);
            if (magic[0]=='R' && magic[1]=='I' && magic[2]=='F' && magic[3]=='F')
                Serial.println("[REC] Header OK (RIFF gevonden)");
            else
                Serial.printf("[REC] WAARSCHUWING: bytes=[%02X %02X %02X %02X]\n",
                              magic[0],magic[1],magic[2],magic[3]);
        }
    }

    Serial.printf("[REC] Klaar: %u frames in %u sec → %s\n",
                  _frameCount, (millis()-_startMs)/1000, _filename);
    return true;
}
