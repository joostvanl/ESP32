#include "web_server.h"
#include "web_pages.h"
#include <SD_MMC.h>
#include <Arduino.h>

WebServerManager::WebServerManager(StorageManager& storage, CameraControl& camera)
    : _server(WEB_PORT), _storage(storage), _camera(camera) {}

void WebServerManager::begin() {
    _server.on("/",         [this](){ handleRoot(); });
    _server.on("/live",     [this](){ handleLive(); });
    _server.on("/videos",   [this](){ handleVideos(); });
    _server.on("/play",     [this](){ handlePlay(); });
    _server.on("/delete",   [this](){ handleDelete(); });
    _server.on("/download", [this](){ handleDownload(); });
    _server.on("/stream",   [this](){ handleStream(); });
    _server.on("/api/status",[this](){ handleApiStatus(); });
    _server.on("/api/videos",[this](){ handleApiVideos(); });
    _server.onNotFound(     [this](){ handleNotFound(); });

    _server.begin();
#if DEBUG_SERIAL
    Serial.printf("[Web] Server gestart op poort %d\n", WEB_PORT);
#endif
}

void WebServerManager::loop() {
    _server.handleClient();
}

// ─── Helper ───────────────────────────────────────────────────────────────────

String WebServerManager::buildFilePath(const String& name) {
    if (name.startsWith("/")) return name;
    return String(VIDEO_DIR) + "/" + name;
}

// ─── Route handlers ───────────────────────────────────────────────────────────

void WebServerManager::handleRoot() {
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send_P(200, "text/html", HTML_DASHBOARD);
}

void WebServerManager::handleLive() {
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send_P(200, "text/html", HTML_LIVE);
}

void WebServerManager::handleVideos() {
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send_P(200, "text/html", HTML_VIDEOS);
}

void WebServerManager::handlePlay() {
    _server.sendHeader("Cache-Control", "no-cache");
    _server.send_P(200, "text/html", HTML_PLAYER);
}

void WebServerManager::handleDelete() {
    if (!_server.hasArg("file")) {
        _server.send(400, "application/json", "{\"ok\":false,\"error\":\"Geen bestandsnaam\"}");
        return;
    }
    String name = _server.arg("file");
    bool ok = _storage.deleteVideo(name.c_str());
#if DEBUG_SERIAL
    Serial.printf("[Web] Verwijder %s: %s\n", name.c_str(), ok ? "OK" : "MISLUKT");
#endif
    _server.send(200, "application/json", ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Niet gevonden\"}");
}

void WebServerManager::handleDownload() {
    if (!_server.hasArg("file")) {
        _server.send(400, "text/plain", "Geen bestandsnaam");
        return;
    }
    String name = _server.arg("file");
    String path = buildFilePath(name);

    File f = SD_MMC.open(path.c_str(), FILE_READ);
    if (!f || f.isDirectory()) {
        _server.send(404, "text/plain", "Bestand niet gevonden");
        return;
    }

    String disposition = "attachment; filename=\"" + name + "\"";
    _server.sendHeader("Content-Disposition", disposition);
    _server.sendHeader("Content-Length", String(f.size()));
    _server.sendHeader("Cache-Control", "no-cache");
    _server.streamFile(f, "video/avi");
    f.close();
}

void WebServerManager::handleStream() {
    if (isRecording) {
        _server.send(503, "text/plain", "Opname bezig, stream niet beschikbaar");
        return;
    }
    if (!_camera.isReady()) {
        _server.send(503, "text/plain", "Camera niet beschikbaar");
        return;
    }

    isStreaming = true;
    WiFiClient client = _server.client();

    // MJPEG stream headers
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
    client.println("Cache-Control: no-cache");
    client.println("Connection: close");
    client.println();

    unsigned long lastFrame = 0;
    const unsigned long frameInterval = 1000 / CAM_FPS_TARGET;

    while (client.connected()) {
        unsigned long now = millis();
        if (now - lastFrame < frameInterval) {
            delay(5);
            continue;
        }
        lastFrame = now;

        camera_fb_t* fb = _camera.captureFrame();
        if (!fb) continue;

        client.printf("--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", fb->len);
        client.write(fb->buf, fb->len);
        client.print("\r\n");
        _camera.releaseFrame(fb);
    }

    isStreaming = false;
#if DEBUG_SERIAL
    Serial.println("[Web] Stream beëindigd");
#endif
}

void WebServerManager::handleApiStatus() {
    auto videos = _storage.listVideos();
    uint64_t free_mb  = _storage.freeBytes()  / (1024*1024);
    uint64_t used_mb  = _storage.usedBytes()  / (1024*1024);
    uint64_t total_mb = _storage.totalBytes() / (1024*1024);

    String json = "{";
    json += "\"camera\":"   + String(_camera.isReady() ? "true" : "false") + ",";
    json += "\"recording\":" + String(isRecording ? "true" : "false") + ",";
    json += "\"streaming\":" + String(isStreaming ? "true" : "false") + ",";
    json += "\"free_mb\":"   + String((uint32_t)free_mb) + ",";
    json += "\"used_mb\":"   + String((uint32_t)used_mb) + ",";
    json += "\"total_mb\":"  + String((uint32_t)total_mb) + ",";
    json += "\"video_count\":" + String(videos.size()) + ",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";

    _server.sendHeader("Cache-Control", "no-cache");
    _server.send(200, "application/json", json);
}

void WebServerManager::handleApiVideos() {
    auto videos = _storage.listVideos();
    uint64_t free_mb  = _storage.freeBytes()  / (1024*1024);
    uint64_t total_mb = _storage.totalBytes() / (1024*1024);

    String json = "{";
    json += "\"free_mb\":"  + String((uint32_t)free_mb) + ",";
    json += "\"total_mb\":" + String((uint32_t)total_mb) + ",";
    json += "\"videos\":[";

    for (size_t i = 0; i < videos.size(); i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"" + videos[i].name + "\",\"size\":\"" + videos[i].sizeStr + "\"}";
    }
    json += "]}";

    _server.sendHeader("Cache-Control", "no-cache");
    _server.send(200, "application/json", json);
}

void WebServerManager::handleNotFound() {
    _server.send(404, "text/plain", "Niet gevonden");
}
