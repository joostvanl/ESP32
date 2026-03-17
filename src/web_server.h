#pragma once
#include <WebServer.h>
#include <WiFi.h>
#include "storage_manager.h"
#include "camera_control.h"
#include "config.h"

class WebServerManager {
public:
    WebServerManager(StorageManager& storage, CameraControl& camera);

    void begin();
    void loop();

    // Externe state die webserver moet kennen
    bool  isRecording  = false;
    bool  isStreaming  = false;
    bool  ledOn        = false;

    // Verwerk eventuele LED-API requests op poort 81 (aanroepen tijdens stream)
    void processLedServer();

private:
    WebServer      _server;
    WiFiServer     _ledServer;
    StorageManager& _storage;
    CameraControl&  _camera;

    // Route handlers
    void handleRoot();
    void handleLive();
    void handleVideos();
    void handlePlay();
    void handleDelete();
    void handleDownload();
    void handleStream();
    void handleThumbnail();
    void handleApiStatus();
    void handleApiVideos();
    void handleApiLed();
    void handleNotFound();

    String buildFilePath(const String& name);
    // Lees het eerste JPEG-frame uit een AVI-bestand
    bool   extractFirstJpeg(const String& aviPath, std::vector<uint8_t>& out);
};
