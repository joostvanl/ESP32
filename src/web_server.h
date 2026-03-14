#pragma once
#include <WebServer.h>
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

private:
    WebServer      _server;
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
    void handleApiStatus();
    void handleApiVideos();
    void handleNotFound();

    String buildFilePath(const String& name);
};
