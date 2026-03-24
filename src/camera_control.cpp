#include "camera_control.h"
#include "app_settings.h"
#include <Arduino.h>

bool CameraControl::begin() {
    camera_config_t cfg;
    cfg.ledc_channel = LEDC_CHANNEL_0;
    cfg.ledc_timer   = LEDC_TIMER_0;
    cfg.pin_d0       = CAM_PIN_D0;
    cfg.pin_d1       = CAM_PIN_D1;
    cfg.pin_d2       = CAM_PIN_D2;
    cfg.pin_d3       = CAM_PIN_D3;
    cfg.pin_d4       = CAM_PIN_D4;
    cfg.pin_d5       = CAM_PIN_D5;
    cfg.pin_d6       = CAM_PIN_D6;
    cfg.pin_d7       = CAM_PIN_D7;
    cfg.pin_xclk     = CAM_PIN_XCLK;
    cfg.pin_pclk     = CAM_PIN_PCLK;
    cfg.pin_vsync    = CAM_PIN_VSYNC;
    cfg.pin_href     = CAM_PIN_HREF;
    cfg.pin_sscb_sda = CAM_PIN_SIOD;
    cfg.pin_sscb_scl = CAM_PIN_SIOC;
    cfg.pin_pwdn     = CAM_PIN_PWDN;
    cfg.pin_reset    = CAM_PIN_RESET;
    cfg.xclk_freq_hz = 20000000;
    cfg.pixel_format = PIXFORMAT_JPEG;
    cfg.frame_size   = appFrameSize();
    cfg.jpeg_quality = CAM_QUALITY;

    // Met PSRAM meer frame buffers voor betere prestaties
    if (psramFound()) {
        cfg.fb_count    = 2;
        cfg.grab_mode   = CAMERA_GRAB_LATEST;
    } else {
        cfg.fb_count    = 1;
        cfg.grab_mode   = CAMERA_GRAB_WHEN_EMPTY;
        // Verlaag resolutie zonder PSRAM
        cfg.frame_size  = FRAMESIZE_CIF;
    }

    esp_err_t err = esp_camera_init(&cfg);
    if (err != ESP_OK) {
#if DEBUG_SERIAL
        Serial.printf("[Camera] Initialisatie mislukt: 0x%x\n", err);
#endif
        return false;
    }

    // Sensorinstellingen optimaliseren voor nestkast (weinig licht)
    sensor_t* s = esp_camera_sensor_get();
    if (s) {
        s->set_brightness(s, 1);
        s->set_contrast(s, 1);
        s->set_saturation(s, 0);
        s->set_gain_ctrl(s, 1);      // auto gain
        s->set_exposure_ctrl(s, 1);  // auto exposure
        s->set_awb_gain(s, 1);       // auto white balance
        s->set_whitebal(s, 1);
        s->set_aec2(s, 1);           // AEC DSP
    }

    // IR LED pin instellen
    pinMode(IR_LED_PIN, OUTPUT);
    digitalWrite(IR_LED_PIN, LOW);

    _ready = true;
#if DEBUG_SERIAL
    Serial.println("[Camera] Geïnitialiseerd");
    if (psramFound()) Serial.println("[Camera] PSRAM beschikbaar");
#endif
    return true;
}

void CameraControl::end() {
    if (_ready) {
        esp_camera_deinit();
        _ready = false;
    }
}

camera_fb_t* CameraControl::captureFrame() {
    if (!_ready) return nullptr;
    return esp_camera_fb_get();
}

void CameraControl::releaseFrame(camera_fb_t* fb) {
    if (fb) esp_camera_fb_return(fb);
}

void CameraControl::setIrLed(bool on) {
    digitalWrite(IR_LED_PIN, on ? HIGH : LOW);
}

bool CameraControl::setFrameSize(framesize_t fs) {
    if (!_ready) return false;
    sensor_t* s = esp_camera_sensor_get();
    if (!s) return false;
    int err = s->set_framesize(s, fs);
    return err == 0;
}
