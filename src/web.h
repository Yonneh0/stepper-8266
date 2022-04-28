
#include "web/cw_png.h"
#include "web/dark_min_js.h"
#include "web/favicon_ico.h"
#include "web/index_htm.h"
#include "web/minstyle_io_css.h"
#include "web/minstyle_io_css_map.h"

void serverSendCache(bool setCache) {
    server.sendHeader("Content-Encoding", "gzip");
    if (setCache)
        server.sendHeader("Cache-Control", "max-age=31536000");
    else
        server.sendHeader("Cache-Control", "max-age=0");
}

void web_h_init() {
    server.on("/", []() { serverSendCache(false); server.send(200, "text/html", index_htm, sizeof(index_htm)); });
    server.on("/favicon.ico", []() { serverSendCache(true); server.send(200, "image/vnd.microsoft.icon", favicon_ico, sizeof(favicon_ico)); });
    server.on("/dark.min.js", []() { serverSendCache(true); server.send(200, "text/javascript", dark_min_js, sizeof(dark_min_js)); });
    server.on("/minstyle.io.css", []() { serverSendCache(true); server.send(200, "text/css", minstyle_io_css, sizeof(minstyle_io_css)); });
    server.on("/minstyle.io.css.map", []() { serverSendCache(true); server.send(200, "application/json", minstyle_io_css_map, sizeof(minstyle_io_css_map)); });
    server.on("/cw.png", []() { serverSendCache(true); server.send(200, "text/css", cw_png, sizeof(cw_png)); });
    server.on(
        "/update", HTTP_POST, []() {
        server.sendHeader("Connection", "close");
        server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
        ESP.restart(); }, []() {
        HTTPUpload& upload = server.upload();
        if (upload.status == UPLOAD_FILE_START) {
            Serial.setDebugOutput(true);
            WiFiUDP::stopAll();
            Serial.printf("Update: %s\n", upload.filename.c_str());
            uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
            if (!Update.begin(maxSketchSpace)) {  // start with max available size
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_WRITE) {
            if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
                Update.printError(Serial);
            }
        } else if (upload.status == UPLOAD_FILE_END) {
            if (Update.end(true)) {  // true to set the size to the current progress
                Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
            } else {
                Update.printError(Serial);
            }
            Serial.setDebugOutput(false);
        }
      yield(); });
    server.begin();
}