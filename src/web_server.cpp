#include "web_server.h"
#include "web_pages.h"
#include "video_recorder.h"
#include <SD_MMC.h>
#include <Arduino.h>
#include <vector>

#if USE_HTTPS
#include <HTTPConnection.hpp>
using namespace httpsserver;
static WebServerManager* g_webManager = nullptr;

static void wrapRoot(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleRootHttps(req, res); }
static void wrapLive(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleLiveHttps(req, res); }
static void wrapVideos(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleVideosHttps(req, res); }
static void wrapPlay(HTTPRequest* req, HTTPResponse* res) { g_webManager->handlePlayHttps(req, res); }
static void wrapDelete(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleDeleteHttps(req, res); }
static void wrapDownload(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleDownloadHttps(req, res); }
static void wrapStream(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleStreamHttps(req, res); }
static void wrapThumbnail(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleThumbnailHttps(req, res); }
static void wrapApiStatus(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleApiStatusHttps(req, res); }
static void wrapApiVideos(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleApiVideosHttps(req, res); }
static void wrapApiLed(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleApiLedHttps(req, res); }
static void wrapNotFound(HTTPRequest* req, HTTPResponse* res) { g_webManager->handleNotFoundHttps(req, res); }
#endif

#if USE_HTTPS
WebServerManager::WebServerManager(StorageManager& storage, CameraControl& camera)
    : _storage(storage), _camera(camera) {}
#else
WebServerManager::WebServerManager(StorageManager& storage, CameraControl& camera)
    : _server(WEB_PORT), _ledServer(LED_API_PORT), _storage(storage), _camera(camera) {}
#endif

WebServerManager::~WebServerManager() {
#if USE_HTTPS
    if (_secureServer) { _secureServer->stop(); delete _secureServer; _secureServer = nullptr; }
    if (_cert) { delete _cert; _cert = nullptr; }
    g_webManager = nullptr;
#endif
}

void WebServerManager::begin() {
#if USE_HTTPS
    g_webManager = this;
    Serial.println("[Web] Self-signed certificaat genereren (kan ~30s duren)...");
    _cert = new SSLCert();
    int r = createSelfSignedCert(*_cert, KEYSIZE_2048, "CN=NestboxCam.local,O=NestboxCam,C=NL", "20200101000000", "20301231235959");
    if (r != 0) {
        Serial.printf("[Web] Certificaat mislukt: 0x%02X\n", r);
        delete _cert; _cert = nullptr;
        return;
    }
    Serial.println("[Web] Certificaat OK");
    _secureServer = new HTTPSServer(_cert, WEB_PORT_HTTPS, 4);
    ResourceNode* nRoot    = new ResourceNode("/", "GET", &wrapRoot);
    ResourceNode* nLive    = new ResourceNode("/live", "GET", &wrapLive);
    ResourceNode* nVideos  = new ResourceNode("/videos", "GET", &wrapVideos);
    ResourceNode* nPlay    = new ResourceNode("/play", "GET", &wrapPlay);
    ResourceNode* nDelete  = new ResourceNode("/delete", "GET", &wrapDelete);
    ResourceNode* nDownload= new ResourceNode("/download", "GET", &wrapDownload);
    ResourceNode* nStream  = new ResourceNode("/stream", "GET", &wrapStream);
    ResourceNode* nThumb   = new ResourceNode("/thumbnail", "GET", &wrapThumbnail);
    ResourceNode* nStatus  = new ResourceNode("/api/status", "GET", &wrapApiStatus);
    ResourceNode* nApiVids = new ResourceNode("/api/videos", "GET", &wrapApiVideos);
    ResourceNode* nLed     = new ResourceNode("/api/led", "GET", &wrapApiLed);
    ResourceNode* n404     = new ResourceNode("", "GET", &wrapNotFound);
    _secureServer->registerNode(nRoot);
    _secureServer->registerNode(nLive);
    _secureServer->registerNode(nVideos);
    _secureServer->registerNode(nPlay);
    _secureServer->registerNode(nDelete);
    _secureServer->registerNode(nDownload);
    _secureServer->registerNode(nStream);
    _secureServer->registerNode(nThumb);
    _secureServer->registerNode(nStatus);
    _secureServer->registerNode(nApiVids);
    _secureServer->registerNode(nLed);
    _secureServer->setDefaultNode(n404);
    _secureServer->start();
#if DEBUG_SERIAL
    Serial.printf("[Web] HTTPS server poort %d\n", WEB_PORT_HTTPS);
#endif
#else
    const char* headerKeys[] = { "Range" };
    _server.collectHeaders(headerKeys, 1);
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
    _server.on("/api/led",   [this](){ handleApiLed(); });
    _server.onNotFound(      [this](){ handleNotFound(); });
    _server.begin();
    _ledServer.begin();
#if DEBUG_SERIAL
    Serial.printf("[Web] Server poort %d, LED-API poort %d\n", WEB_PORT, LED_API_PORT);
#endif
#endif
}

void WebServerManager::loop() {
#if USE_HTTPS
    if (_secureServer) _secureServer->loop();
#else
    _server.handleClient();
    processLedServer();
#endif
}


// ─── Helper ───────────────────────────────────────────────────────────────────

String WebServerManager::buildFilePath(const String& name) {
    if (name.startsWith("/")) return name;
    return String(VIDEO_DIR) + "/" + name;
}

// ─── Route handlers (HTTP) ────────────────────────────────────────────────────

#if !USE_HTTPS
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

    size_t totalSize = f.size();
    String safeName = name;
    int lastSlash = name.lastIndexOf('/');
    if (lastSlash >= 0) safeName = name.substring(lastSlash + 1);
    String disposition = "inline; filename=\"" + safeName + "\"";
    _server.sendHeader("Content-Disposition", disposition);
    _server.sendHeader("Cache-Control", "no-cache");
    _server.sendHeader("X-Content-Type-Options", "nosniff");
    _server.sendHeader("Accept-Ranges", "bytes");

    // Range request (voor videostreaming in de browser)
    String rangeStr = _server.hasHeader("Range") ? _server.header("Range") : "";
    size_t rangeStart = 0, rangeEnd = totalSize - 1;
    bool useRange = false;
    if (rangeStr.startsWith("bytes=")) {
        rangeStr = rangeStr.substring(6);
        int dash = rangeStr.indexOf('-');
        if (dash >= 0) {
            rangeStart = (size_t)rangeStr.substring(0, dash).toInt();
            if (dash + 1 < (int)rangeStr.length())
                rangeEnd = (size_t)rangeStr.substring(dash + 1).toInt();
            else
                rangeEnd = totalSize - 1;
            if (rangeEnd >= totalSize) rangeEnd = totalSize - 1;
            if (rangeStart <= rangeEnd) useRange = true;
        }
    }

    if (useRange) {
        size_t partLen = rangeEnd - rangeStart + 1;
        _server.sendHeader("Content-Range", "bytes " + String(rangeStart) + "-" + String(rangeEnd) + "/" + String(totalSize));
        _server.sendHeader("Content-Length", String(partLen));
        _server.setContentLength(partLen);
        _server.send(206, "video/x-msvideo", "");
        if (!f.seek(rangeStart, SeekSet)) {
            f.close();
            return;
        }
        uint8_t buf[512];
        while (partLen && f.available()) {
            size_t toRead = (partLen < sizeof(buf)) ? partLen : sizeof(buf);
            size_t n = f.read(buf, toRead);
            if (n == 0) break;
            _server.client().write(buf, n);
            partLen -= n;
        }
    } else {
        _server.sendHeader("Content-Length", String(totalSize));
        _server.streamFile(f, "video/x-msvideo");
    }
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
        processLedServer();   // LED-knop op poort 81 werkt tijdens stream

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
    // LED uitzetten als die aan stond voor de stream
    if (ledOn) {
        ledOn = false;
        _camera.setIrLed(false);
    }
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
#endif

// ─── Thumbnail: eerste JPEG-frame uit AVI ────────────────────────────────────
// AVI-structuur: RIFF header (252 bytes) → "00dc" chunk → JPEG data

bool WebServerManager::extractFirstJpeg(const String& aviPath, std::vector<uint8_t>& out) {
    File f = SD_MMC.open(aviPath.c_str(), FILE_READ);
    if (!f || f.size() < 260) return false;

    // Scan maximaal 8 KB om het eerste "00dc" chunk te vinden.
    // Frames beginnen op offset VideoRecorder::HEADER_SIZE (252).
    const size_t FRAME_START = VideoRecorder::HEADER_SIZE;
    const size_t SCAN = 8192;
    uint8_t* buf = (uint8_t*)malloc(SCAN);
    if (!buf) { f.close(); return false; }

    size_t rd = f.read(buf, min((size_t)f.size(), SCAN));
    f.close();

    bool found = false;
    for (size_t i = FRAME_START; i + 8 <= rd; i++) {
        if (buf[i]=='0' && buf[i+1]=='0' && buf[i+2]=='d' && buf[i+3]=='c') {
            uint32_t len = (uint32_t)buf[i+4]
                         | ((uint32_t)buf[i+5] << 8)
                         | ((uint32_t)buf[i+6] << 16)
                         | ((uint32_t)buf[i+7] << 24);
            if (len < 32 || len > 150000) { i += 3; continue; }

            size_t dataOff = i + 8;
            if (dataOff + len <= rd) {
                // JPEG zit volledig in het gescande venster
                out.assign(buf + dataOff, buf + dataOff + len);
                found = true;
            } else {
                // Lees de rest van het bestand voor dit frame
                free(buf);
                File f2 = SD_MMC.open(aviPath.c_str(), FILE_READ);
                if (!f2) return false;
                f2.seek(dataOff);
                out.resize(len);
                size_t got = f2.read(out.data(), len);
                f2.close();
                return got == len;
            }
            break;
        }
    }
    free(buf);
    return found;
}

#if !USE_HTTPS
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
    _server.sendHeader("Content-Length", String(jpeg.size()));
    _server.setContentLength(jpeg.size());
    _server.send(200, "image/jpeg", "");
    _server.client().write(jpeg.data(), jpeg.size());
}

void WebServerManager::processLedServer() {
    WiFiClient client = _ledServer.available();
    if (!client || !client.connected()) return;

    String req;
    while (client.available()) {
        char c = client.read();
        if (c == '\n' || c == '\r') break;
        req += c;
        if (req.length() > 120) break;
    }
    while (client.available()) client.read();

    if (req.indexOf("GET /api/led") >= 0) {
        if (req.indexOf("state=on") >= 0)      ledOn = true;
        else if (req.indexOf("state=off") >= 0) ledOn = false;
        else if (req.indexOf("state=toggle") >= 0) ledOn = !ledOn;
        _camera.setIrLed(ledOn);
#if DEBUG_SERIAL
        Serial.printf("[Web] LED (poort 81) %s\n", ledOn ? "aan" : "uit");
#endif
    }

    String json = "{\"led\":" + String(ledOn ? "true" : "false") + "}";
    client.println("HTTP/1.1 200 OK");
    client.println("Content-Type: application/json");
    client.println("Connection: close");
    client.println("Cache-Control: no-cache");
    client.print("Content-Length: "); client.println(json.length());
    client.println();
    client.print(json);
    client.flush();
    client.stop();
}

void WebServerManager::handleApiLed() {
    _server.sendHeader("Access-Control-Allow-Origin", "*");
    _server.sendHeader("Cache-Control", "no-cache");

    String state = _server.arg("state");
    if (state == "on") {
        ledOn = true;
    } else if (state == "off") {
        ledOn = false;
    } else if (state == "toggle") {
        ledOn = !ledOn;
    }

    _camera.setIrLed(ledOn);
#if DEBUG_SERIAL
    Serial.printf("[Web] LED %s\n", ledOn ? "aan" : "uit");
#endif

    _server.send(200, "application/json",
                 String("{\"led\":") + (ledOn ? "true" : "false") + "}");
}
#endif

// ─── HTTPS handlers ───────────────────────────────────────────────────────────

#if USE_HTTPS
static void sendProgmem(HTTPResponse* res, const char* pgm) {
    if (!pgm) return;
    size_t len = strlen_P(pgm);
    for (size_t i = 0; i < len; i++)
        res->write((uint8_t)pgm_read_byte(pgm + i));
}

void WebServerManager::handleRootHttps(HTTPRequest* req, HTTPResponse* res) {
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "text/html; charset=utf-8");
    sendProgmem(res, HTML_DASHBOARD);
}

void WebServerManager::handleLiveHttps(HTTPRequest* req, HTTPResponse* res) {
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "text/html; charset=utf-8");
    sendProgmem(res, HTML_LIVE);
}

void WebServerManager::handleVideosHttps(HTTPRequest* req, HTTPResponse* res) {
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "text/html; charset=utf-8");
    sendProgmem(res, HTML_VIDEOS);
}

void WebServerManager::handlePlayHttps(HTTPRequest* req, HTTPResponse* res) {
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "text/html; charset=utf-8");
    sendProgmem(res, HTML_PLAYER);
}

void WebServerManager::handleDeleteHttps(HTTPRequest* req, HTTPResponse* res) {
    std::string val;
    if (!req->getParams()->getQueryParameter("file", val)) {
        res->setStatusCode(400);
        res->setHeader("Content-Type", "application/json");
        res->printStd("{\"ok\":false,\"error\":\"Geen bestandsnaam\"}");
        return;
    }
    String name = String(val.c_str());
    bool ok = _storage.deleteVideo(name.c_str());
#if DEBUG_SERIAL
    Serial.printf("[Web] Verwijder %s: %s\n", name.c_str(), ok ? "OK" : "MISLUKT");
#endif
    res->setHeader("Content-Type", "application/json");
    res->printStd(ok ? "{\"ok\":true}" : "{\"ok\":false,\"error\":\"Niet gevonden\"}");
}

void WebServerManager::handleDownloadHttps(HTTPRequest* req, HTTPResponse* res) {
    std::string val;
    if (!req->getParams()->getQueryParameter("file", val)) {
        res->setStatusCode(400);
        res->setHeader("Content-Type", "text/plain");
        res->printStd("Geen bestandsnaam");
        return;
    }
    String name = String(val.c_str());
    String path = buildFilePath(name);
    File f = SD_MMC.open(path.c_str(), FILE_READ);
    if (!f || f.isDirectory()) {
        res->setStatusCode(404);
        res->setHeader("Content-Type", "text/plain");
        res->printStd("Bestand niet gevonden");
        return;
    }
    size_t totalSize = f.size();
    int lastSlash = name.lastIndexOf('/');
    String safeName = lastSlash >= 0 ? name.substring(lastSlash + 1) : name;
    res->setHeader("Content-Disposition", ("inline; filename=\"" + safeName + "\"").c_str());
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("X-Content-Type-Options", "nosniff");
    res->setHeader("Accept-Ranges", "bytes");

    std::string rangeStr = req->getHeader("Range");
    size_t rangeStart = 0, rangeEnd = totalSize - 1;
    bool useRange = false;
    if (rangeStr.find("bytes=") == 0) {
        rangeStr = rangeStr.substr(6);
        size_t dash = rangeStr.find('-');
        if (dash != std::string::npos) {
            rangeStart = (size_t)atoi(rangeStr.substr(0, dash).c_str());
            if (dash + 1 < rangeStr.length())
                rangeEnd = (size_t)atoi(rangeStr.substr(dash + 1).c_str());
            else
                rangeEnd = totalSize - 1;
            if (rangeEnd >= totalSize) rangeEnd = totalSize - 1;
            if (rangeStart <= rangeEnd) useRange = true;
        }
    }

    if (useRange) {
        size_t partLen = rangeEnd - rangeStart + 1;
        res->setStatusCode(206);
        res->setStatusText("Partial Content");
        res->setHeader("Content-Range", ("bytes " + String(rangeStart) + "-" + String(rangeEnd) + "/" + String(totalSize)).c_str());
        res->setHeader("Content-Length", String(partLen).c_str());
        res->setHeader("Content-Type", "video/x-msvideo");
        if (f.seek(rangeStart, SeekSet)) {
            uint8_t buf[512];
            while (partLen && f.available()) {
                size_t toRead = (partLen < sizeof(buf)) ? partLen : sizeof(buf);
                size_t n = f.read(buf, toRead);
                if (n == 0) break;
                res->write(buf, n);
                partLen -= n;
            }
        }
    } else {
        res->setHeader("Content-Length", String(totalSize).c_str());
        res->setHeader("Content-Type", "video/x-msvideo");
        uint8_t buf[512];
        while (f.available()) {
            size_t n = f.read(buf, sizeof(buf));
            if (n == 0) break;
            res->write(buf, n);
        }
    }
    f.close();
}

void WebServerManager::handleStreamHttps(HTTPRequest* req, HTTPResponse* res) {
    if (isRecording) {
        res->setStatusCode(503);
        res->setHeader("Content-Type", "text/plain");
        res->printStd("Opname bezig, stream niet beschikbaar");
        return;
    }
    if (!_camera.isReady()) {
        res->setStatusCode(503);
        res->setHeader("Content-Type", "text/plain");
        res->printStd("Camera niet beschikbaar");
        return;
    }
    isStreaming = true;
    res->setHeader("Content-Type", "multipart/x-mixed-replace; boundary=frame");
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Connection", "close");

    unsigned long lastFrame = 0;
    const unsigned long frameInterval = 1000 / CAM_FPS_TARGET;
    HTTPConnection* conn = (HTTPConnection*)res->_con;

    while (conn && !conn->isClosed() && !conn->isError()) {
        unsigned long now = millis();
        if (now - lastFrame >= frameInterval) {
            lastFrame = now;
            camera_fb_t* fb = _camera.captureFrame();
            if (fb && fb->len > 0) {
                char hdr[80];
                snprintf(hdr, sizeof(hdr), "--frame\r\nContent-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n", (unsigned)fb->len);
                res->write((const uint8_t*)hdr, strlen(hdr));
                res->write(fb->buf, fb->len);
                res->write((const uint8_t*)"\r\n", 2);
                _camera.releaseFrame(fb);
            } else if (fb) {
                _camera.releaseFrame(fb);
            }
        } else {
            delay(5);
        }
    }

    isStreaming = false;
    if (ledOn) {
        ledOn = false;
        _camera.setIrLed(false);
    }
#if DEBUG_SERIAL
    Serial.println("[Web] Stream beëindigd");
#endif
}

void WebServerManager::handleThumbnailHttps(HTTPRequest* req, HTTPResponse* res) {
    std::string val;
    if (!req->getParams()->getQueryParameter("file", val)) {
        res->setStatusCode(400);
        res->setHeader("Content-Type", "text/plain");
        res->printStd("Geen bestandsnaam");
        return;
    }
    String name = String(val.c_str());
    String path = buildFilePath(name);
    std::vector<uint8_t> jpeg;
    if (!extractFirstJpeg(path, jpeg) || jpeg.empty()) {
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
        res->setHeader("Cache-Control", "max-age=60");
        res->setHeader("Content-Type", "image/jpeg");
        res->write(EMPTY_JPEG, sizeof(EMPTY_JPEG));
        return;
    }
    res->setHeader("Cache-Control", "max-age=300");
    res->setHeader("Content-Type", "image/jpeg");
    res->setHeader("Content-Length", String(jpeg.size()).c_str());
    res->write(jpeg.data(), jpeg.size());
}

void WebServerManager::handleApiStatusHttps(HTTPRequest* req, HTTPResponse* res) {
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
    if (!videos.empty()) json += "\"last_video\":\"" + videos[0].name + "\",";
    else json += "\"last_video\":\"\",";
    json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
    json += "}";
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "application/json");
    res->print(json.c_str());
}

void WebServerManager::handleApiVideosHttps(HTTPRequest* req, HTTPResponse* res) {
    auto videos = _storage.listVideos();
    uint64_t free_mb  = _storage.freeBytes()  / (1024*1024);
    uint64_t total_mb = _storage.totalBytes() / (1024*1024);
    String json = "{\"free_mb\":"  + String((uint32_t)free_mb) + ",\"total_mb\":" + String((uint32_t)total_mb) + ",\"videos\":[";
    for (size_t i = 0; i < videos.size(); i++) {
        if (i > 0) json += ",";
        json += "{\"name\":\"" + videos[i].name + "\",\"size\":\"" + videos[i].sizeStr + "\"}";
    }
    json += "]}";
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "application/json");
    res->print(json.c_str());
}

void WebServerManager::handleApiLedHttps(HTTPRequest* req, HTTPResponse* res) {
    std::string val;
    if (req->getParams()->getQueryParameter("state", val)) {
        if (val == "on") ledOn = true;
        else if (val == "off") ledOn = false;
        else if (val == "toggle") ledOn = !ledOn;
    }
    _camera.setIrLed(ledOn);
#if DEBUG_SERIAL
    Serial.printf("[Web] LED %s\n", ledOn ? "aan" : "uit");
#endif
    res->setHeader("Cache-Control", "no-cache");
    res->setHeader("Content-Type", "application/json");
    res->print(ledOn ? "{\"led\":true}" : "{\"led\":false}");
}

void WebServerManager::handleNotFoundHttps(HTTPRequest* req, HTTPResponse* res) {
    req->discardRequestBody();
    res->setStatusCode(404);
    res->setStatusText("Not Found");
    res->setHeader("Content-Type", "text/plain");
    res->printStd("Niet gevonden");
}
#endif
