#include "app_settings.h"
#include <ctype.h>

static bool keyMatch(const char* s, const char* ref) {
    if (!s) return false;
    while (*ref) {
        if (tolower((unsigned char)*s++) != (unsigned char)*ref++) return false;
    }
    return *s == 0;
}

static uint32_t     s_recordSec = RECORD_DURATION_SEC;
static framesize_t  s_frameSize = CAM_FRAME_SIZE;

void appSettingsInit() {
    s_recordSec = RECORD_DURATION_SEC;
    s_frameSize = CAM_FRAME_SIZE;
}

uint32_t appRecordDurationSec() {
    return s_recordSec;
}

bool appSetRecordDurationSec(uint32_t sec) {
    if (sec != 10 && sec != 20 && sec != 30 && sec != 60) return false;
    s_recordSec = sec;
    return true;
}

framesize_t appFrameSize() {
    return s_frameSize;
}

bool appSetFrameSize(framesize_t fs) {
    if (fs != FRAMESIZE_VGA && fs != FRAMESIZE_SVGA && fs != FRAMESIZE_XGA) return false;
    s_frameSize = fs;
    return true;
}

const char* appFrameSizeKey() {
    if (s_frameSize == FRAMESIZE_SVGA) return "svga";
    if (s_frameSize == FRAMESIZE_XGA)  return "xga";
    return "vga";
}

bool appSetFrameSizeFromKey(const char* key) {
    if (!key) return false;
    if (keyMatch(key, "vga"))  return appSetFrameSize(FRAMESIZE_VGA);
    if (keyMatch(key, "svga")) return appSetFrameSize(FRAMESIZE_SVGA);
    if (keyMatch(key, "xga") || keyMatch(key, "xvga"))
        return appSetFrameSize(FRAMESIZE_XGA);
    return false;
}

void framesizeToPixels(framesize_t fs, uint16_t& w, uint16_t& h) {
    switch (fs) {
        case FRAMESIZE_SVGA: w = 800;  h = 600;  break;
        case FRAMESIZE_XGA:  w = 1024; h = 768;  break;
        default:             w = 640;  h = 480;  break;
    }
}
