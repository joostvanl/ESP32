#include "web_server.h"
#include "web_pages.h"
#include <SD_MMC.h>
#include <Arduino.h>
#include <vector>

WebServerManager::WebServerManager(StorageManager& storage, CameraControl& camera)
    : _server(WEB_PORT), _storage(storage), _camera(camera) {}

void WebServerManager::begin() {
    _server.on("/",          [this](){ handleRoot(); });
    _server.on("/live",      [this](){ handleLive(); });
    _server.on("/videos",    [this](){ handleVideos(); });
    _server.on("/play",      [this](){ handlePlay(); });
    _server.on("/delete",    [this](){ handleDelete(); });
    _server.on("/download",  [this](){ handleDownload(); });
    _server.on("/stream",    [this](){ handleStream(); });
    _server.on("/thumbnail", [this](){ handleThumbnail(); });
    _server.on("/api/status",[this](){ handleApiStatus(); });
    _server.on("/api/videos",[this](){ handleApiVideos(); });
    _server.onNotFound(      [this](){ handleNotFound(); });

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
    if (!videos.empty()) {
        json += "\"last_video\":\"" + videos[0].name + "\",";
    } else {
        json += "\"last_video\":\"\",";
    }
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

// ─── Thumbnail: eerste JPEG-frame uit AVI ────────────────────────────────────
// AVI-structuur: RIFF header (240 bytes) → LIST movi → "00dc" chunk → JPEG data

bool WebServerManager::extractFirstJpeg(const String& aviPath, std::vector<uint8_t>& out) {
    File f = SD_MMC.open(aviPath.c_str(), FILE_READ);
    if (!f || f.size() < 260) return false;

    // Zoek "00dc" fourcc na offset 248 (start van movi data)
    // We scannen maximaal 4 KB om snel te zijn
    const size_t SCAN = 4096;
    uint8_t buf[SCAN];
    size_t read = f.read(buf, min((size_t)f.size(), SCAN));
    f.close();

    for (size_t i = 248; i + 8 < read; i++) {
        if (buf[i]=='0' && buf[i+1]=='0' && buf[i+2]=='d' && buf[i+3]=='c') {
            uint32_t len = (uint32_t)buf[i+4] | ((uint32_t)buf[i+5]<<8)
                         | ((uint32_t)buf[i+6]<<16) | ((uint32_t)buf[i+7]<<24);
            if (len < 8 || len > 200000) continue;
            size_t dataOff = i + 8;
            if (dataOff + len > read) {
                // Frame loopt buiten gescand venster – lees opnieuw
                File f2 = SD_MMC.open(aviPath.c_str(), FILE_READ);
                if (!f2) return false;
                f2.seek(dataOff);
                out.resize(len);
                size_t got = f2.read(out.data(), len);
                f2.close();
                return got == len;
            }
            out.assign(buf + dataOff, buf + dataOff + len);
            return true;
        }
    }
    return false;
}

void WebServerManager::handleThumbnail() {
    if (!_server.hasArg("file")) {
        _server.send(400, "text/plain", "Geen bestandsnaam");
        return;
    }
    String name = _server.arg("file");
    String path = buildFilePath(name);

    std::vector<uint8_t> jpeg;
    if (!extractFirstJpeg(path, jpeg) || jpeg.empty()) {
        // Stuur een 1x1 transparante pixel terug als fallback
        static const uint8_t EMPTY_JPEG[] = {
            0xFF,0xD8,0xFF,0xE0,0x00,0x10,0x4A,0x46,0x49,0x46,0x00,0x01,
            0x01,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0xFF,0xDB,0x00,0x43,
            0x00,0x08,0x06,0x06,0x07,0x06,0x05,0x08,0x07,0x07,0x07,0x09,
            0x09,0x08,0x0A,0x0C,0x14,0x0D,0x0C,0x0B,0x0B,0x0C,0x19,0x12,
            0x13,0x0F,0x14,0x1D,0x1A,0x1F,0x1E,0x1D,0x1A,0x1C,0x1C,0x20,
            0x24,0x2E,0x27,0x20,0x22,0x2C,0x23,0x1C,0x1C,0x28,0x37,0x29,
            0x2C,0x30,0x31,0x34,0x34,0x34,0x1F,0x27,0x39,0x3D,0x38,0x32,
            0x3C,0x2E,0x33,0x34,0x32,0xFF,0xC0,0x00,0x0B,0x08,0x00,0x01,
            0x00,0x01,0x01,0x01,0x11,0x00,0xFF,0xC4,0x00,0x1F,0x00,0x00,
            0x01,0x05,0x01,0x01,0x01,0x01,0x01,0x01,0x00,0x00,0x00,0x00,
            0x00,0x00,0x00,0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,
            0x09,0x0A,0x0B,0xFF,0xC4,0x00,0xB5,0x10,0x00,0x02,0x01,0x03,
            0x03,0x02,0x04,0x03,0x05,0x05,0x04,0x04,0x00,0x00,0x01,0x7D,
            0x01,0x02,0x03,0x00,0x04,0x11,0x05,0x12,0x21,0x31,0x41,0x06,
            0x13,0x51,0x61,0x07,0x22,0x71,0x14,0x32,0x81,0x91,0xA1,0x08,
            0x23,0x42,0xB1,0xC1,0x15,0x52,0xD1,0xF0,0x24,0x33,0x62,0x72,
            0x82,0xFF,0xDA,0x00,0x08,0x01,0x01,0x00,0x00,0x3F,0x00,0xFB,
            0xD2,0x8A,0x28,0x03,0xFF,0xD9
        };
        _server.sendHeader("Cache-Control", "max-age=60");
        _server.send_P(200, "image/jpeg", (const char*)EMPTY_JPEG, sizeof(EMPTY_JPEG));
        return;
    }

    _server.sendHeader("Cache-Control", "max-age=300");
    _server.send(200, "image/jpeg", (const char*)jpeg.data(), jpeg.size());
}
