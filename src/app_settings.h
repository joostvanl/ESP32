#pragma once
#include <Arduino.h>
#include "esp_camera.h"
#include "config.h"

void appSettingsInit();

uint32_t appRecordDurationSec();
bool     appSetRecordDurationSec(uint32_t sec);  // alleen 10,20,30,60

framesize_t appFrameSize();
bool        appSetFrameSize(framesize_t fs);
const char* appFrameSizeKey();  // "vga" | "svga" | "xga"

bool appSetFrameSizeFromKey(const char* key);
void framesizeToPixels(framesize_t fs, uint16_t& w, uint16_t& h);
