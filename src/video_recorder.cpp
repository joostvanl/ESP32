#include "video_recorder.h"
#include <stdio.h>   // fopen / fwrite / fclose

// ─── Constructor / destructor ─────────────────────────────────────────────────

VideoRecorder::VideoRecorder() {
    _index = new AviIdx[MAX_INDEX];
}

VideoRecorder::~VideoRecorder() {
    if (_recording) finish();
    delete[] _index;
}

// ─── Schrijfhelpers ───────────────────────────────────────────────────────────

static void u32le(uint8_t* p, uint32_t v) {
    p[0]=v; p[1]=v>>8; p[2]=v>>16; p[3]=v>>24;
}
static void u16le(uint8_t* p, uint16_t v) {
    p[0]=v; p[1]=v>>8;
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

// ─── AVI header in buffer bouwen ─────────────────────────────────────────────
//
// Layout: 248 bytes, frames starten op byte 248
//   [  0] RIFF <size> AVI
//   [ 12] LIST <212>  hdrl
//   [ 24]   avih(56)          → totalFrames @ byte 48
//   [ 88] LIST <132>  strl
//   [100]   strh(56)          → length      @ byte 140
//   [164]   strf(40)
//   [212] JUNK(20)            (28 bytes totaal, padding)
//   [240] LIST <size> movi
//   [248] <frames>
//
void VideoRecorder::buildAviHeader(uint8_t* b, uint32_t w, uint32_t h,
                                    uint32_t frames, uint32_t moviData) {
    memset(b, 0, HEADER_SIZE);
    uint32_t fps   = CAM_FPS_TARGET;
    uint32_t movi  = 4 + moviData;                    // "movi" + frame chunks
    uint32_t idx   = 8 + frames * 16;                 // "idx1" chunk
    uint32_t riff  = (HEADER_SIZE - 8) + moviData + idx;

    memcpy(b,    "RIFF", 4); u32le(b+4,  riff);
    memcpy(b+8,  "AVI ", 4);
    memcpy(b+12, "LIST", 4); u32le(b+16, 212);
    memcpy(b+20, "hdrl", 4);

    // avih @ 24
    memcpy(b+24, "avih", 4); u32le(b+28, 56);
    u32le(b+32, 1000000/fps);  // microSecPerFrame
    u32le(b+36, 0);
    u32le(b+40, 0);
    u32le(b+44, 0x10);         // AVIF_HASINDEX
    u32le(b+48, frames);       // totalFrames  ← patch target
    u32le(b+52, 0);
    u32le(b+56, 1);            // streams
    u32le(b+60, w*h*3);
    u32le(b+64, w);
    u32le(b+68, h);

    // LIST strl @ 88
    memcpy(b+88, "LIST", 4); u32le(b+92, 132);
    memcpy(b+96, "strl", 4);

    // strh @ 100
    memcpy(b+100, "strh", 4); u32le(b+104, 56);
    memcpy(b+108, "vids", 4);
    memcpy(b+112, "MJPG", 4);
    u32le(b+116, 0);
    u16le(b+120, 0); u16le(b+122, 0);
    u32le(b+124, 0);
    u32le(b+128, 1);           // scale
    u32le(b+132, fps);         // rate
    u32le(b+136, 0);
    u32le(b+140, frames);      // length       ← patch target
    u32le(b+144, w*h*3);
    u32le(b+148, 0xFFFFFFFF);  // quality
    u32le(b+152, 0);
    u16le(b+156, 0); u16le(b+158, 0);
    u16le(b+160, (uint16_t)w);
    u16le(b+162, (uint16_t)h);

    // strf @ 164
    memcpy(b+164, "strf", 4); u32le(b+168, 40);
    u32le(b+172, 40);
    u32le(b+176, w);
    u32le(b+180, h);
    u16le(b+184, 1);           // biPlanes
    u16le(b+186, 24);          // biBitCount
    memcpy(b+188, "MJPG", 4);
    u32le(b+192, w*h*3);

    // JUNK @ 212  (28 bytes: 4+4+20 payload nullen)
    memcpy(b+212, "JUNK", 4); u32le(b+216, 20);

    // LIST movi @ 240
    memcpy(b+240, "LIST", 4); u32le(b+244, movi);
    memcpy(b+244, "LIST", 4); u32le(b+248-8, movi);
    // Correcte schrijfvolgorde:
    u32le(b+244, movi);        // LIST size
    memcpy(b+248-4, "movi", 4);
}

// ─── Opname starten ───────────────────────────────────────────────────────────

bool VideoRecorder::begin(const char* filename) {
    if (_recording) finish();

    strncpy(_filename, filename, sizeof(_filename)-1);
    _filename[sizeof(_filename)-1] = '\0';
    _frameCount = 0;
    _moviSize   = 0;
    _startMs    = millis();

    Serial.printf("[REC] begin() → %s\n", _filename);
    Serial.printf("[REC] Vrij heap: %u bytes\n", (uint32_t)ESP.getFreeHeap());

    // SD-kaart mount punt voor fopen
    // SD_MMC.begin("/sdcard", ...) → VFS pad = /sdcard + bestandsnaam
    snprintf(_vfsPath, sizeof(_vfsPath), "/sdcard%s", _filename);
    Serial.printf("[REC] VFS pad: %s\n", _vfsPath);

    _file = SD_MMC.open(filename, FILE_WRITE);
    if (!_file) {
        Serial.printf("[REC] FOUT: SD_MMC.open mislukt voor %s\n", filename);
        return false;
    }
    Serial.printf("[REC] Bestand geopend, positie=%u\n", (uint32_t)_file.position());

    // Placeholder header schrijven (framecount=0, zal later gepatcht worden)
    uint8_t hdr[HEADER_SIZE];
    buildAviHeader(hdr, CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT, 0, 0);
    size_t wr = _file.write(hdr, HEADER_SIZE);

    Serial.printf("[REC] Header geschreven: %u/%u bytes, positie=%u\n",
                  wr, (size_t)HEADER_SIZE, (uint32_t)_file.position());

    if (wr != HEADER_SIZE) {
        Serial.println("[REC] FOUT: Header schrijven onvolledig!");
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
        Serial.println("[REC] writeFrame: niet opgenomen (recording=false of geen file)");
        return false;
    }
    if (len == 0) {
        Serial.println("[REC] writeFrame: len=0, overgeslagen");
        return false;
    }

    uint32_t posBefore = (uint32_t)_file.position();
    uint32_t frameOffset = posBefore - HEADER_SIZE;

    writeFourCC("00dc");
    writeU32LE((uint32_t)len);
    size_t wr = _file.write(data, len);

    if (wr != len) {
        Serial.printf("[REC] FOUT frame #%u: gevraagd=%u geschreven=%u pos=%u\n",
                      _frameCount, (uint32_t)len, (uint32_t)wr, posBefore);
        return false;
    }

    if (len & 1) { uint8_t pad=0; _file.write(&pad,1); }

    if (_frameCount < MAX_INDEX) {
        _index[_frameCount] = { frameOffset, (uint32_t)len };
    }
    _moviSize += 8 + (uint32_t)len + (len & 1);
    _frameCount++;

    // Log elke 10 frames
    if (_frameCount == 1 || _frameCount % 10 == 0) {
        Serial.printf("[REC] Frame #%u: %u bytes, movi=%u, filepos=%u\n",
                      _frameCount, (uint32_t)len, _moviSize,
                      (uint32_t)_file.position());
    }
    return true;
}

// ─── Opname stoppen ───────────────────────────────────────────────────────────

bool VideoRecorder::finish() {
    if (!_recording || !_file) {
        Serial.println("[REC] finish() aangeroepen maar niet bezig met opname");
        return false;
    }
    _recording = false;

    Serial.printf("[REC] finish() – %u frames, moviSize=%u\n", _frameCount, _moviSize);

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
    uint32_t finalPos = (uint32_t)_file.position();
    Serial.printf("[REC] idx1 geschreven, eindpositie=%u\n", finalPos);

    _file.close();
    Serial.println("[REC] File gesloten");

    // ── Verificatie: controleer bestandsgrootte via SD_MMC ─────────────────
    {
        File check = SD_MMC.open(_filename, FILE_READ);
        if (check) {
            Serial.printf("[REC] Bestandsgrootte na close: %u bytes\n", (uint32_t)check.size());
            check.close();
        } else {
            Serial.println("[REC] FOUT: kan bestand niet openen voor verificatie!");
        }
    }

    // ── Header patchen via fopen "r+b" (GEEN truncate) ─────────────────────
    // FILE_WRITE in ESP32 Arduino SD_MMC = "w" = truncate naar 0!
    // fopen met "r+b" opent voor lezen+schrijven ZONDER te truncaten.
    Serial.printf("[REC] Header patchen via fopen: %s\n", _vfsPath);

    FILE* fp = fopen(_vfsPath, "r+b");
    if (!fp) {
        Serial.printf("[REC] FOUT: fopen r+b mislukt voor %s (errno=%d)\n",
                      _vfsPath, errno);
        // Probeer alternatief VFS pad zonder /sdcard prefix
        Serial.println("[REC] Probeer pad zonder /sdcard prefix...");
        fp = fopen(_filename, "r+b");
        if (!fp) {
            Serial.printf("[REC] FOUT: ook dat mislukt (errno=%d)\n", errno);
            Serial.println("[REC] Video opgeslagen ZONDER correcte header (kan mogelijk nog afgespeeld worden)");
            return false;
        }
    }

    uint8_t hdr[HEADER_SIZE];
    buildAviHeader(hdr, CAM_FRAME_WIDTH, CAM_FRAME_HEIGHT, _frameCount, _moviSize);
    fseek(fp, 0, SEEK_SET);
    size_t wr = fwrite(hdr, 1, HEADER_SIZE, fp);
    fclose(fp);

    Serial.printf("[REC] Header patch: %u/%u bytes geschreven\n", wr, (size_t)HEADER_SIZE);

    // ── Finale verificatie ─────────────────────────────────────────────────
    {
        File check = SD_MMC.open(_filename, FILE_READ);
        if (check) {
            uint32_t sz = (uint32_t)check.size();
            // Lees de eerste 4 bytes om te controleren of het "RIFF" is
            uint8_t magic[4];
            check.read(magic, 4);
            check.close();
            Serial.printf("[REC] Finale bestandsgrootte: %u bytes\n", sz);
            Serial.printf("[REC] Eerste 4 bytes: %c%c%c%c\n",
                          magic[0], magic[1], magic[2], magic[3]);
            if (magic[0]=='R' && magic[1]=='I' && magic[2]=='F' && magic[3]=='F') {
                Serial.println("[REC] Header OK (RIFF gevonden)");
            } else {
                Serial.println("[REC] WAARSCHUWING: Header lijkt onjuist!");
            }
        }
    }

    uint32_t elapsed = (millis() - _startMs) / 1000;
    Serial.printf("[REC] Opname voltooid: %u frames in %u sec → %s\n",
                  _frameCount, elapsed, _filename);
    return true;
}
