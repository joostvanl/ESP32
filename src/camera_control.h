#pragma once
#include "esp_camera.h"
#include "config.h"

class CameraControl {
public:
    bool begin();
    void end();

    // Haal één frame op. Caller moet fb vrijgeven met releaseFrame()
    camera_fb_t* captureFrame();
    void         releaseFrame(camera_fb_t* fb);

    bool isReady() const { return _ready; }

    // Schakel IR LED aan/uit
    void setIrLed(bool on);

private:
    bool _ready = false;
};
