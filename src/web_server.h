#pragma once
#include "storage_manager.h"
#include "camera_control.h"
#include "config.h"

#if USE_HTTPS
#include <HTTPSServer.hpp>
#include <SSLCert.hpp>
#include <HTTPRequest.hpp>
#include <HTTPResponse.hpp>
#include <ResourceNode.hpp>
#include <string>
#include <WiFi.h>
#else
#include <WebServer.h>
#include <WiFi.h>
#endif

class WebServerManager {
public:
    WebServerManager(StorageManager& storage, CameraControl& camera);
    ~WebServerManager();

    void begin();
    void loop();

    bool  isRecording  = false;
    bool  isStreaming  = false;
    bool  ledOn        = false;

    void processLedServer();

#if USE_HTTPS
    void handleRootHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleLiveHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleVideosHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handlePlayHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleDeleteHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleDownloadHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleStreamHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleThumbnailHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleApiStatusHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleApiVideosHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleApiLedHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleApiSettingsHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleApiCaptureHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
    void handleNotFoundHttps(httpsserver::HTTPRequest* req, httpsserver::HTTPResponse* res);
#endif

private:
#if USE_HTTPS
    httpsserver::SSLCert*       _cert = nullptr;
    httpsserver::HTTPSServer*   _secureServer = nullptr;
#else
    WebServer      _server;
    WiFiServer     _ledServer;
#endif
    StorageManager& _storage;
    CameraControl&  _camera;

#if !USE_HTTPS
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
    void handleApiSettings();
    void handleApiCapture();
    void handleNotFound();
#endif

    String buildFilePath(const String& name);
    bool   extractFirstJpeg(const String& aviPath, std::vector<uint8_t>& out);
    bool   readJpegFile(const String& path, std::vector<uint8_t>& out);
};
